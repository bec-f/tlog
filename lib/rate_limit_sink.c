#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <tlog/rate_limit_sink.h>

/** Rate Limit sink instance */
struct rate_limit_sink {
    struct tlog_sink        sink;       /**< Abstract sink instance */
    struct tlog_sink       *dsink;      /**< "Destination" (logging) sink */
    int                     rate;       /**< Cutoff rate */
}

static bool
tog_rate_limit_sink_is_valid(const struct tlog_sink *sink)
{
    struct tlog_rate_limit_sink *rate_limit_sink =
                                (struct tlog_rate_limit_sink *)sink;
    return rate_limit_sink != NULL &&
           tlog_sink_is_valid(rate_limit_sink->dsink);
}

static void
tlog_rate_limit_sink_cleanup(struct tlog_sink *sink)
{
    struct tlog_rate_limit_sink *rate_limit_sink =
                                (struct tlog_rate_limit_sink *)sink;
    rate_limit_sink->dsink = NULL;
    (void) rate_limit_sink; /* unused unless asserts are enabled */

    assert(rate_limit_sink != NULL);
}

static tlog_grc
tlog_rate_limit_sink_init(struct tlog_sink *sink, va_list ap)
{
    struct tlog_rate_limit_sink *rate_limit_sink =
                                 (struct tlog_rate_limit_sink *)sink;
    struct tlog_sink *pdsink = va_arg(ap, struct tlog_sink *);

    if(dsink == NULL){
      goto error;
    }
    rate_limit_sink->dsink = pdsink;
    rate_limit_sink->rate = va_arg(ap, int)
    return TLOG_RC_OK;

error:
    tlog_rate_limit_sink_cleanup(sink);
    return TLOG_GRC_ERRNO;
}

static tlog_grc
tlog_sink_write(struct tlog_sink *sink,
                const struct tlog_pkt *pkt,
                struct tlog_pkt_pos *ppos,
                const struct tlog_pkt_pos *end)
{
    struct tlog_rate_limit_sink *rate_limit_sink =
                                 (struct tlog_rate_limit_sink *)sink;
    tlog_grc grc;

    if(pkt->type != TLOG_PKT_TYPE_IO || rate_limit_sink->rate == 0){
        grc = tlog_sink_write(rate_limit_sink->dsink, &pkt, &log_pos, NULL);
        if (grc != TLOG_RC_OK &&
            grc != TLOG_GRC_FROM(errno, EINTR)) {

            tlog_errs_pushc(perrs, grc);
            tlog_errs_pushs(perrs, "Failed logging terminal data");
            return_grc = grc;
            goto error;
        }
    }
    else {
        int cur_rate = //Calulate rate. Not sure how to do yet
        if(cur_rate < rate_limit_sink->rate) {
            tlog_sink_write(rate_limit_sink->dsink, &pkt, &log_pos, NULL);
        }
        else {
            tlog_pkt_pos_move_past(rate_limit_sink->dsink, &pkt);
        }
    }
}



const struct tlog_sink_type tlog_rate_limit_sink_type = {
    .size       = sizeof(struct tlog_rate_limit_sink),
    .init       = tlog_rate_limit_sink_init,
    .cleanup    = tlog_rate_limit_sink_cleanup,
    .is_valid   = tlog_rate_limit_sink_is_valid,
    .write      = tlog_rate_limit_sink_write,
};

