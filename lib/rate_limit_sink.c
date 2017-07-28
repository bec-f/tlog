#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <tlog/pkt.h>
#include <tlog/sink.h>
#include <tlog/grc.h>
#include <tlog/rc.h>
#include <tlog/rate_limit_sink.h>

/** Rate Limit sink instance */
struct tlog_rate_limit_sink {
    struct tlog_sink        sink;           /**< Abstract sink instance */
    struct tlog_sink       *dsink;          /**< "Destination" (logging) sink */
    bool                    dsink_owned;    /**< True if dsink is owned */
    uint64_t                rate;           /**< Cutoff rate */

    struct tlog_pkt       *prev_pkt;       /**< Previous packet to check */
    struct tlog_pkt_pos   *prev_ppos;           /**< Previous position for writing */
    struct tlog_pkt_pos   *prev_end;            /**< Previous end pos for writing */

    struct timespec        prev_ts;       /**< Previous timestamp for time elapsed */
};

static bool
tlog_rate_limit_sink_is_valid(const struct tlog_sink *sink)
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
    assert(rate_limit_sink != NULL);
    if(rate_limit_sink->dsink_owned == true){
        tlog_sink_destroy(rate_limit_sink->dsink);
        rate_limit_sink->dsink_owned = false;
    }
    rate_limit_sink->prev_pkt = NULL;
    rate_limit_sink->prev_ppos = NULL;
    rate_limit_sink->prev_end = NULL;
    (void) rate_limit_sink; /* unused unless asserts are enabled */
}

static tlog_grc
tlog_rate_limit_sink_init(struct tlog_sink *sink, va_list ap)
{
    struct tlog_rate_limit_sink *rate_limit_sink =
                                 (struct tlog_rate_limit_sink *)sink;
    struct tlog_sink *pdsink = va_arg(ap, struct tlog_sink *);

    if(pdsink == NULL){
      goto error;
    }
    rate_limit_sink->dsink = pdsink;
    rate_limit_sink->dsink_owned = (bool)va_arg(ap, int);
    rate_limit_sink->rate = va_arg(ap, uint64_t);
    rate_limit_sink->prev_pkt = NULL;
    rate_limit_sink->prev_ppos = NULL;
    rate_limit_sink->prev_end = NULL;

    /*TRACING/DEBUGGING CODE: uncomment for a "program trace" file*/
    FILE *f;
    f = fopen("/var/lib/tlog/trace_sink.txt", "a");
    if(f == NULL){
        printf("problem opening file0\n");
        exit(1);
    }
    time_t t;
    time(&t);
    struct tm *timer;
    timer = localtime(&t);
    char buf[20];
    strftime(buf, sizeof(buf), "%b %d %T", timer);
    fprintf(f, "Sink initialized, rate = %lu; [%s]\n", rate_limit_sink->rate, buf);
    fclose(f);
    //END TRACING CODE

    return TLOG_RC_OK;

error:
    tlog_rate_limit_sink_cleanup(sink);
    return TLOG_GRC_ERRNO;
}

    static tlog_grc
tlog_rate_limit_sink_write(struct tlog_sink *sink,
        const struct tlog_pkt *pkt,
        struct tlog_pkt_pos *ppos,
        const struct tlog_pkt_pos *end)
{
    struct tlog_rate_limit_sink *rate_limit_sink =
        (struct tlog_rate_limit_sink *)sink;
    tlog_grc grc;

    if(rate_limit_sink->prev_pkt == NULL) {
        grc = TLOG_RC_OK;
        goto end;
    }

    if(rate_limit_sink->prev_pkt->type != TLOG_PKT_TYPE_IO || rate_limit_sink->rate == 0){

        /*TRACING/DEBUGGING CODE: uncomment for a "program trace" file*/
        FILE *f;
        f = fopen("/var/lib/tlog/trace_sink.txt", "a");
        if(f == NULL){
            printf("problem opening file1\n");
            exit(1);
        }
        time_t t;
        time(&t);
        struct tm *timer;
        timer = localtime(&t);
        char buf[20];
        strftime(buf, sizeof(buf), "%b %d %T", timer);
        fprintf(f, "Sink write, wrong pkt type or rate=0; [%s]\n", buf);
        fclose(f);
        //END TRACING CODE

        grc = tlog_sink_write(rate_limit_sink->dsink, rate_limit_sink->prev_pkt,
                rate_limit_sink->prev_ppos, rate_limit_sink->prev_end);
        goto end;
    }
    else {

        /*TRACING/DEBUGGING CODE: uncomment for a "program trace" file*/
        FILE *f;
        f = fopen("/var/lib/tlog/trace_sink.txt", "a");
        if(f == NULL){
            printf("problem opening file1\n");
            exit(1);
        }
        fprintf(f, "old_pkt timestamp: %lld.%.9ld,pos: %d; new_pkt timestamp: %lld.%.9ld,pos: %d\n", (long long)rate_limit_sink->prev_ts.tv_sec, rate_limit_sink->prev_ts.tv_nsec, rate_limit_sink->prev_ppos, (long long)pkt->timestamp.tv_sec, pkt->timestamp.tv_nsec, *ppos);
        fclose(f);
        //END TRACING CODE


        uint64_t time_change1 = (pkt->timestamp.tv_sec - rate_limit_sink->prev_ts.tv_sec) * 1000000;
        uint64_t time_change2 = (pkt->timestamp.tv_nsec - rate_limit_sink->prev_ts.tv_nsec)/1000;
        uint64_t time_change = time_change1 + time_change2;
        /* Get the rate as an integer with decimal precision */
        double d_rate = (double)(rate_limit_sink->prev_pkt->data.io.len)/(double)(time_change);
        uint64_t cur_rate = d_rate * 1000000;

        /*TRACING/DEBUGGING CODE: uncomment for a "program trace" file*/
        f = fopen("/var/lib/tlog/trace_sink.txt", "a");
        if(f == NULL){
            printf("problem opening file1\n");
            exit(1);
        }
        time_t t;
        time(&t);
        struct tm *timer;
        timer = localtime(&t);
        char buf[20];
        strftime(buf, sizeof(buf), "%b %d %T", timer);
        fprintf(f, "Sink write, looking at %d bytes: %s\n", rate_limit_sink->prev_pkt->data.io.len, rate_limit_sink->prev_pkt->data.io.buf, buf);
        fprintf(f, "For above, sink-rate= %lu, cur-rate=%lu, time elapsed:%u+%u=%u; [%s]\n", rate_limit_sink->rate, cur_rate, time_change1, time_change2, time_change, buf);
        fclose(f);
        //END TRACING CODE

        if(cur_rate < rate_limit_sink->rate) {

            /*TRACING/DEBUGGING CODE: uncomment for a "program trace" file*/
            FILE *f;
            f = fopen("/var/lib/tlog/trace_sink.txt", "a");
            if(f == NULL){
                printf("problem opening file1\n");
                exit(1);
            }
            time_t t;
            time(&t);
            struct tm *timer;
            timer = localtime(&t);
            char buf[20];
            strftime(buf, sizeof(buf), "%b %d %T", timer);
            fprintf(f, "Sink write, writing to rate_limit_sink; [%s]\n", buf);
            fclose(f);
            //END TRACING CODE*/

            grc = tlog_sink_write(rate_limit_sink->dsink, rate_limit_sink->prev_pkt,
                    rate_limit_sink->prev_ppos, rate_limit_sink->prev_end);
            goto end;
        }
        else {

            /*TRACING/DEBUGGING CODE: uncomment for a "program trace" file*/
            FILE *f;
            f = fopen("/var/lib/tlog/trace_sink.txt", "a");
            if(f == NULL){
                printf("problem opening file1\n");
                exit(1);
            }
            time_t t;
            time(&t);
            struct tm *timer;
            timer = localtime(&t);
            char buf[20];
            strftime(buf, sizeof(buf), "%b %d %T", timer);
            fprintf(f, "Sink write, skipping pkt; [%s]\n", buf);
            fclose(f);
            //END TRACING CODE */

            tlog_pkt_pos_move_past(rate_limit_sink->prev_ppos, rate_limit_sink->prev_pkt);
            return TLOG_RC_OK;
        }
    }

end:
    rate_limit_sink->prev_pkt = pkt;
    rate_limit_sink->prev_ppos = ppos;
    rate_limit_sink->prev_end = end;
    rate_limit_sink->prev_ts = pkt->timestamp;
    return grc;
}

    static tlog_grc
tlog_rate_limit_sink_cut(struct tlog_sink *sink)
{
    struct tlog_rate_limit_sink *rate_limit_sink =
                                (struct tlog_rate_limit_sink *)sink;
    tlog_grc grc;

    grc = tlog_sink_cut(rate_limit_sink->dsink);
    return grc;
}

    static tlog_grc
tlog_rate_limit_sink_flush(struct tlog_sink *sink)
{
    struct tlog_rate_limit_sink *rate_limit_sink =
                                 (struct tlog_rate_limit_sink *)sink;
    tlog_grc grc;

    grc = tlog_sink_flush(rate_limit_sink->dsink);
    return grc;
}



const struct tlog_sink_type tlog_rate_limit_sink_type = {
    .size       = sizeof(struct tlog_rate_limit_sink),
    .init       = tlog_rate_limit_sink_init,
    .cleanup    = tlog_rate_limit_sink_cleanup,
    .is_valid   = tlog_rate_limit_sink_is_valid,
    .write      = tlog_rate_limit_sink_write,
    .cut        = tlog_rate_limit_sink_cut,
    .flush      = tlog_rate_limit_sink_flush,
};
