#include "sunxi-mmc.h"

static int sdxc_read_data_pio(struct sunxi_mmc_host *host, struct mci_data *data);
static int sdxc_write_data_pio(struct sunxi_mmc_host *host, struct mci_data *data);
static int sunxi_mmc_send_cmd(struct sunxi_mmc_host *host, struct mci_cmd *cmd, struct mci_data *data, const char **why);
static int sunxi_mmc_set_ios(struct sunxi_mmc_host *host, struct mci_ios *ios);
static void sunxi_mmc_init(struct sunxi_mmc_host *host);

static int sdxc_read_data_pio(struct sunxi_mmc_host *host, struct mci_data *data)
{
       size_t i, len = data->blocks * data->blocksize;
       u8 *dst = data->dest;
       u32 reg;

       sdxc_writel(host, SDXC_REG_GCTRL, SDXC_GCTRL_ACCESS_BY_AHB);

       for (i = 0; i < len; i += 4) {
               if (wait_on_timeout(2000 * MSECOND, !sdxc_is_fifo_empty(host)))
                       return -ETIMEDOUT;
               reg = sdxc_readl(host, SDXC_REG_FIFO);
               memcpy(dst + i, &reg, sizeof(reg));
       }

       return i;
}

static int sdxc_write_data_pio(struct sunxi_mmc_host *host, struct mci_data *data)
{
       size_t i, len = data->blocks * data->blocksize;
       u32 *pdata = (u32 *)data->src;

       sdxc_writel(host, SDXC_REG_GCTRL, SDXC_GCTRL_ACCESS_BY_AHB);

       for (i = 0; i < len; i += 4, pdata++) {
               if (wait_on_timeout(2000 * MSECOND, !sdxc_is_fifo_full(host)))
                       return -ETIMEDOUT;
               sdxc_writel(host, SDXC_REG_FIFO, *pdata);
       }
#if 0
       sdxc_writel(host, SDXC_RINTR, SDXC_INTMSK_TXDR);

       if (wait_on_timeout(2000 * MSECOND, sdxc_is_fifo_empty(host))) {
               return -EIO;
       }
#endif
       return i;
}

static int sunxi_mmc_send_cmd(struct sunxi_mmc_host *host, struct mci_cmd *cmd,
                             struct mci_data *data, const char **why)
{
       const char *err_why = "";
       u32 cmdval = SDXC_CMD_START;
       int ret;

       if ((cmd->resp_type & MMC_RSP_136) && (cmd->resp_type & MMC_RSP_BUSY))
               return -EINVAL;

       if (cmd->cmdidx == MMC_CMD_STOP_TRANSMISSION)
               return 0; /* using ACMD12 */
       if (cmd->cmdidx == MMC_CMD_GO_IDLE_STATE)
               cmdval |= SDXC_CMD_SEND_INIT_SEQ;

       if (cmd->resp_type & MMC_RSP_PRESENT)
               cmdval |= SDXC_CMD_RESP_EXPIRE;
       if (cmd->resp_type & MMC_RSP_136)
               cmdval |= SDXC_CMD_LONG_RESPONSE;
       if (cmd->resp_type & MMC_RSP_CRC)
               cmdval |= SDXC_CMD_CHK_RESPONSE_CRC;

       /* clear interrupts */
       sdxc_writel(host, SDXC_REG_RINTR, 0xffffffff);

       if (data) {
               u32 blksiz = data->blocksize;
               u32 bytcnt = data->blocks * data->blocksize;

               cmdval |= SDXC_CMD_DATA_EXPIRE;
               cmdval |= SDXC_CMD_WAIT_PRE_OVER;
               if (data->flags & MMC_DATA_WRITE)
                       cmdval |= SDXC_CMD_WRITE;
               if (data->blocks > 1)
                       cmdval |= SDXC_CMD_AUTO_STOP;

               sdxc_writel(host, SDXC_REG_TMOUT, 0xFFFFFF40);
               sdxc_writel(host, SDXC_REG_BLKSZ, blksiz);
               sdxc_writel(host, SDXC_REG_BCNTR, bytcnt);
       }

       sdxc_writel(host, SDXC_REG_CARG, cmd->cmdarg);
       sdxc_writel(host, SDXC_REG_CMDR, cmdval | cmd->cmdidx);
       if (data) {
               if (data->flags & MMC_DATA_WRITE)
                       ret = sdxc_write_data_pio(host, data);
               else
                       ret = sdxc_read_data_pio(host, data);
               if (ret < 0) {
                       err_why = "pio error";
                       goto err;
               }
       }

       ret = sdxc_xfer_complete(host, 1000 * MSECOND, SDXC_INT_COMMAND_DONE);
       if (ret) {
               err_why = "cmd timeout";
               goto err;
       }

       if (data) {
               ret = sdxc_xfer_complete(host, 1000 * MSECOND, data->blocks > 1 ?
                                        SDXC_INT_AUTO_COMMAND_DONE :
                                        SDXC_INT_DATA_OVER);
               if (ret) {
                       err_why = "data timeout";
                       goto err;
               }
       }

       if (cmd->resp_type & MMC_RSP_BUSY) {
               u32 status;
               u64 start;
               start = get_time_ns();
               do {
                       status = sdxc_readl(host, SDXC_REG_STAS);
                       if (is_timeout(start, 2000 * MSECOND)) {
                               err_why = "resp timeout";
                               ret = -ETIMEDOUT;
                               goto err;
                       }
               } while (status & SDXC_STATUS_BUSY);
       }

       if (wait_on_timeout(1000 * MSECOND, !sdxc_is_card_busy(host))) {
               err_why = "card busy timeout";
               ret = -ETIMEDOUT;
               goto err;
       }

       if (cmd->resp_type & MMC_RSP_136) {
               cmd->response[0] = sdxc_readl(host, SDXC_REG_RESP3);
               cmd->response[1] = sdxc_readl(host, SDXC_REG_RESP2);
               cmd->response[2] = sdxc_readl(host, SDXC_REG_RESP1);
               cmd->response[3] = sdxc_readl(host, SDXC_REG_RESP0);
       } else if (cmd->resp_type & MMC_RSP_PRESENT) {
               cmd->response[0] = sdxc_readl(host, SDXC_REG_RESP0);
       }

err:
       if (why)
               *why = err_why;
       sdxc_writel(host, SDXC_REG_GCTRL, SDXC_GCTRL_FIFO_RESET);
       return ret;
}

static int sunxi_mmc_update_clk(struct sunxi_mmc_host *host)
{
       u32 cmdval;

       cmdval = SDXC_CMD_START |
                SDXC_CMD_UPCLK_ONLY |
                SDXC_CMD_WAIT_PRE_OVER;

       sdxc_writel(host, SDXC_REG_CARG, 0);
       sdxc_writel(host, SDXC_REG_CMDR, cmdval);

       if (wait_on_timeout(2000 * MSECOND, !(sdxc_readl(host, SDXC_REG_CMDR) & SDXC_CMD_START)))
               return -ETIMEDOUT;

       return 0;
}

static int sunxi_mmc_setup_clk(struct sunxi_mmc_host *host, u32 freq)
{
       u32 val, div, sclk;
       int ret;

       sclk = host->clkrate;
       if (sclk == 0)
               return -EINVAL;

       sclk /= 2; // WHY ????

       /* disable card clock */
       val = sdxc_readl(host, SDXC_REG_CLKCR);
       val &= ~(SDXC_CLK_ENABLE | SDXC_CLK_LOW_POWER_ON);
       val |= SDXC_CLK_MASK_DATA0;
       sdxc_writel(host, SDXC_REG_CLKCR, val);

       ret = sunxi_mmc_update_clk(host);
       if (ret)
               return ret;

       /*
        * Configure the controller to use the new timing mode if needed.
        * On controllers that only support the new timing mode, such as
        * the eMMC controller on the A64, this register does not exist,
        * and any writes to it are ignored.
        */
       if (host->cfg->needs_new_timings) {
               /* Don't touch the delay bits */
               val = sdxc_readl(host, SDXC_REG_NTSR);
               val |= SDXC_NTSR_2X_TIMING_MODE;
               sdxc_writel(host, SDXC_REG_NTSR, val);
       }

       /* setup clock rate */
       div = DIV_ROUND_UP(sclk, freq);
       if (div > 510)
               div = 510;

       /* set internal divider */
       val = sdxc_readl(host, SDXC_REG_CLKCR);
       val &= ~SDXC_CLK_DIVIDER_MASK;
       val |= div / 2; /* divisor is multiply by 2 */
       sdxc_writel(host, SDXC_REG_CLKCR, val);

       /* enable card clock */
       val = sdxc_readl(host, SDXC_REG_CLKCR);
       val |= SDXC_CLK_ENABLE;
       val &= ~SDXC_CLK_MASK_DATA0;
       sdxc_writel(host, SDXC_REG_CLKCR, val);

       return sunxi_mmc_update_clk(host);
}

static int sunxi_mmc_set_ios(struct sunxi_mmc_host *host, struct mci_ios *ios)
{
       int ret = 0;
       u32 width;

       switch (ios->bus_width) {
       case MMC_BUS_WIDTH_8:
               width = SDXC_WIDTH_8BIT;
               break;
       case MMC_BUS_WIDTH_4:
               width = SDXC_WIDTH_4BIT;
               break;
       default:
               width = SDXC_WIDTH_1BIT;
               break;
       }
       sdxc_writel(host, SDXC_REG_WIDTH, width);

       if (ios->clock)
               ret = sunxi_mmc_setup_clk(host, ios->clock);
       return ret;
}

static void sunxi_mmc_init(struct sunxi_mmc_host *host)
{
       /* Reset controller */
       sdxc_writel(host, SDXC_REG_GCTRL, SDXC_GCTRL_RESET);
       udelay(1000);

       sdxc_writel(host, SDXC_REG_RINTR, 0xffffffff);
       sdxc_writel(host, SDXC_REG_IMASK, 0);

       sdxc_writel(host, SDXC_REG_TMOUT, 0xffffff40);
}
