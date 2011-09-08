/*
 * ICD - Intelligent Call Distributor 
 *
 * Copyright (C) 2003, 2004, 2005
 *
 * Written by Anthony Minessale II <anthmct at yahoo dot com>
 * Written by Bruce Atherton <bruce at callenish dot com>
 * Additions, Changes and Support by Tim R. Clark <tclark at shaw dot ca>
 * Changed to adopt to jabber interaction and adjusted for CallWeaver.org by
 * Halo Kwadrat Sp. z o.o., Piotr Figurny and Michal Bielicki
 * 
 * This application is a part of:
 * 
 * CallWeaver -- An open source telephony toolkit.
 * Copyright (C) 1999 - 2005, Digium, Inc.
 * Mark Spencer <markster@digium.com>
 *
 * See http://www.callweaver.org for more information about
 * the CallWeaver project. Please do not directly contact
 * any of the maintainers of this project for assistance;
 * the project provides a web site, mailing lists and IRC
 * channels for your use.
 *
 * This program is free software, distributed under the terms of
 * the GNU General Public License Version 2. See the LICENSE file
 * at the top of the source tree.
 */
 
 /*! \file
  * \brief icd_conference.c  -  conferencing and conference type bridging features
*/

#ifdef HAVE_CONFIG_H
#include "confdefs.h"
#endif 

#include <assert.h>
#include "callweaver/icd/icd_types.h"
#include "callweaver/icd/icd_common.h"
#include "callweaver/icd/icd_conference.h"
#include "callweaver/icd/icd_caller.h"
#include "callweaver/icd/icd_caller_list.h"
#include "callweaver/icd/icd_caller_private.h"
#include <sys/ioctl.h>
#include <errno.h>
#include "callweaver/icd/conf_enter.h"
#include "callweaver/icd/conf_leave.h"

#define ENTER   0
#define LEAVE   1

/* conference mode presets */
#define CONF_SIZE 160
#define CONF_MODE_REGULAR 772
#define CONF_MODE_MUTE 260

 CW_MUTEX_DEFINE_STATIC(conflock);
static void_hash_table *CONF_REGISTRY;
static int GLOBAL_USAGE = 0;

static int open_pseudo_zap(void);

static int careful_write(int fd, unsigned char *data, int len)
{
    int res;

    while (len) {
        res = write(fd, data, len);
        if (res < 1) {
            if (errno != EAGAIN) {
                cw_log(LOG_WARNING, "Failed to write audio data to conference: %s\n", strerror(errno));
                return -1;
            } else
                return 0;
        }
        len -= res;
        data += res;
    }
    return 0;
}

static int activate_conference(int fd, int confno, int flags)
{
    struct zt_confinfo ztc;

    memset(&ztc, 0, sizeof(ztc));
    ztc.chan = 0;
    ztc.confno = confno;
    ztc.confmode = flags;

    if (ioctl(fd, ZT_SETCONF, &ztc)) {
        cw_log(LOG_WARNING, "Error setting conference\n");
        close(fd);
        return 0;
    }
    return 1;
}

static int wipe_conference(int fd)
{
    struct zt_confinfo ztc;

    memset(&ztc, 0, sizeof(ztc));
    ztc.chan = 0;
    ztc.confno = 0;
    ztc.confmode = 0;
    if (ioctl(fd, ZT_SETCONF, &ztc)) {
        cw_log(LOG_WARNING, "Error setting conference\n");
        return 0;
    }
    return 1;
}

static void conf_play(icd_conference * conf, int sound)
{
    unsigned char *data;
    int len;

    cw_mutex_lock(&conflock);
    switch (sound) {
    case ENTER:
        data = enter;
        len = sizeof(enter);
        break;
    case LEAVE:
        data = leave;
        len = sizeof(leave);
        break;
    default:
        data = NULL;
        len = 0;
    }
    if (data)
        careful_write(conf->fd, data, len);
    cw_mutex_unlock(&conflock);
}

static int open_pseudo_zap()
{
    int fd = 0, flags = 0;
    ZT_BUFFERINFO bi;

    cw_log(LOG_WARNING, "gonna open pseudo channel:\n");
    fd = open("/dev/zap/pseudo", O_RDWR);
    if (fd < 0) {
        cw_log(LOG_WARNING, "Unable to open pseudo channel: %s\n", strerror(errno));
        return 0;
    }
    /* Make non-blocking */
    flags = fcntl(fd, F_GETFL);
    if (flags < 0) {
        cw_log(LOG_WARNING, "Unable to get flags: %s\n", strerror(errno));
        close(fd);
        return 0;
    }
    if (fcntl(fd, F_SETFL, flags | O_NONBLOCK)) {
        cw_log(LOG_WARNING, "Unable to set flags: %s\n", strerror(errno));
        close(fd);
        return 0;
    }
    /* Setup buffering information */
    memset(&bi, 0, sizeof(bi));
    bi.bufsize = CONF_SIZE;
    bi.txbufpolicy = ZT_POLICY_IMMEDIATE;
    bi.rxbufpolicy = ZT_POLICY_IMMEDIATE;
    bi.numbufs = 4;
    if (ioctl(fd, ZT_SET_BUFINFO, &bi)) {
        cw_log(LOG_WARNING, "Unable to set buffering information: %s\n", strerror(errno));
        close(fd);
        return 0;
    }

    return fd;

}

void icd_conference__init_registry()
{
    if (CONF_REGISTRY)
        return;
    CONF_REGISTRY = vh_init("CONFERENCES");
}

void icd_conference__destroy_registry()
{
    if (CONF_REGISTRY)
        vh_destroy(&CONF_REGISTRY);

}

int icd_conference__usecount(icd_conference * conf)
{
    return conf->usecount;
}

icd_status icd_conference__register(char *name, icd_conference * conf)
{
    icd_conference__init_registry();
    vh_write(CONF_REGISTRY, name, conf);
    return ICD_SUCCESS;
}

icd_status icd_conference__deregister(char *name)
{
    icd_conference__init_registry();
    vh_delete(CONF_REGISTRY, name);
    return ICD_SUCCESS;
}

icd_conference *icd_conference__locate(char *name)
{
    icd_conference__init_registry();
    return vh_read(CONF_REGISTRY, name);
}

vh_keylist *icd_conference__list()
{
    icd_conference__init_registry();
    return vh_keys(CONF_REGISTRY);
}

icd_conference *icd_conference__new(char *name)
{
    icd_conference *new;

    ICD_MALLOC(new, sizeof(icd_conference));

    strncpy(new->name, name, sizeof(new->name));
    new->usecount = 0;
    time(&new->start);
    new->owner = NULL;
    new->is_agent_conf = 0;
    new->fd = open("/dev/zap/pseudo", O_RDWR);
    if (new->fd < 0) {
        cw_log(LOG_WARNING, "Unable to open pseudo channel\n");
        ICD_STD_FREE(new);
        new = NULL;
    } else {
        memset(&new->ztc, 0, sizeof(new->ztc));
        new->ztc.chan = 0;
        new->ztc.confno = -1;
        new->ztc.confmode = ZT_CONF_CONF | ZT_CONF_TALKER | ZT_CONF_LISTENER;
        if (ioctl(new->fd, ZT_SETCONF, &new->ztc)) {
            cw_log(LOG_WARNING, "Error setting conference\n");
            close(new->fd);
            ICD_STD_FREE(new);
            new = NULL;
        }
    }

    return new;
}

int icd_conference__set_global_usage(int value)
{
    GLOBAL_USAGE = value ? 1 : 0;
    return GLOBAL_USAGE;
}

int icd_conference__get_global_usage(void)
{
    return GLOBAL_USAGE;
}

icd_status icd_conference__clear(icd_caller * that)
{
    struct icd_conference *conf;

    assert(that != NULL);
    conf = that->conference;

    if (conf) {
        if (conf->owner && (conf->owner == that)) {
            cw_log(LOG_NOTICE, "Goodbye Conf %d\n", conf->ztc.confno);
            close(conf->fd);
            ICD_STD_FREE(conf);
            conf = NULL;
        }
    } else
        return ICD_STDERR;

    that->conference = NULL;

    return ICD_SUCCESS;
}

icd_status icd_conference__associate(icd_caller * that, icd_conference * conf, int owner)
{
    assert(that != NULL);
    assert(conf != NULL);

    if (that->conference == conf) {
        return ICD_SUCCESS;
    }

    if (owner) {
        if (conf->owner) {
            cw_log(LOG_WARNING, "Error setting conference owner, one already exists...\n");
        } else {
            conf->owner = that;
            conf->is_agent_conf = 1;
        }
    }
    that->conference = conf;

    return (that->conference == NULL) ? ICD_STDERR : ICD_SUCCESS;
}

icd_status icd_conference__join(icd_caller * that)
{

    int fd = 0, nfds = 0, outfd = 0, ms = 0, origfd, ret = 0, flags = 0, confno = 0, res = 0;
    struct cw_frame *read_frame;
    struct cw_channel *active_channel;
    struct cw_channel *chan = NULL;
    icd_conference *conf;

    /*  CW_FORMAT_ULAW  CW_FORMAT_SLINEAR this requires ioctl on the conference fd  */
    int icd_conf_format = CW_FORMAT_SLINEAR;

    int x;
    int pseudo_fd=0;
    ZT_BUFFERINFO bi;
    char __buf[CONF_SIZE +  CW_FRIENDLY_OFFSET];
    char *buf = __buf +  CW_FRIENDLY_OFFSET;
    struct cw_frame write_frame;

    conf = that->conference;
    chan = that->chan;

    if (!conf || !chan || !conf->ztc.confno) {
        cw_log(LOG_ERROR, "Invalid conference....\n");
        if(icd_caller__has_flag(that, ICD_MONITOR_FLAG)){
	     icd_caller__set_state(that, ICD_CALLER_STATE_CALL_END);
	}
	else {
	     icd_caller__set_state(that, ICD_CALLER_STATE_BRIDGE_FAILED);
	}
        return ICD_STDERR;
    }
    
    cw_indicate(chan, -1);

    /* Set it into linear mode (write) */
    if (cw_set_write_format(chan, icd_conf_format) < 0) {
        cw_log(LOG_WARNING, "Unable to set '%s' to write correct audio codec mode[%d]\n", chan->name,
            icd_conf_format);
        if(icd_caller__has_flag(that, ICD_MONITOR_FLAG)){
	     icd_caller__set_state(that, ICD_CALLER_STATE_CALL_END);
	}
	else {
	     icd_caller__set_state(that, ICD_CALLER_STATE_BRIDGE_FAILED);
	}
        return ICD_STDERR;
    }

    /* Set it into linear mode (read) */
    if (cw_set_read_format(chan, icd_conf_format) < 0) {
        if(icd_caller__has_flag(that, ICD_MONITOR_FLAG)){
	     icd_caller__set_state(that, ICD_CALLER_STATE_CALL_END);
	}
	else {
	     icd_caller__set_state(that, ICD_CALLER_STATE_BRIDGE_FAILED);
	}
        cw_log(LOG_WARNING, "Unable to set '%s' to read correct audio codec mode[%d]\n", chan->name,
            icd_conf_format);
        return ICD_STDERR;
    }

    cw_log(LOG_NOTICE, "Joining conference....%d\n", conf->ztc.confno);

    origfd = chan->fds[0];

    if (that->conf_fd && (fcntl(that->conf_fd, F_GETFL) > -1)) {
        fd = that->conf_fd;
        nfds = strcmp(chan->type, "DAHDI") ? 1 : 0;
    } else {
        if (strcmp(chan->type, "DAHDI")) {
            fd = open_pseudo_zap();
            if (!fd) {
                cw_log(LOG_ERROR, "Can't create pseudo channel...\n");
        	if(icd_caller__has_flag(that, ICD_MONITOR_FLAG)){
	     		icd_caller__set_state(that, ICD_CALLER_STATE_CALL_END);
		}
		else {
	     		icd_caller__set_state(that, ICD_CALLER_STATE_BRIDGE_FAILED);
		}
                return ICD_STDERR;
            }
            x = 1;
            pseudo_fd=1;
            /* Make non-blocking */
            flags = fcntl(fd, F_GETFL);
            if (flags < 0) {
                cw_log(LOG_WARNING, "Unable to get flags: %s\n", strerror(errno));
                close(fd);
        	if(icd_caller__has_flag(that, ICD_MONITOR_FLAG)){
	     		icd_caller__set_state(that, ICD_CALLER_STATE_CALL_END);
		}
		else {
	     		icd_caller__set_state(that, ICD_CALLER_STATE_BRIDGE_FAILED);
		}
                return ICD_STDERR;
            }
            if (fcntl(fd, F_SETFL, flags | O_NONBLOCK)) {
                cw_log(LOG_WARNING, "Unable to set flags: %s\n", strerror(errno));
                close(fd);
        	if(icd_caller__has_flag(that, ICD_MONITOR_FLAG)){
	     		icd_caller__set_state(that, ICD_CALLER_STATE_CALL_END);
		}
		else {
	     		icd_caller__set_state(that, ICD_CALLER_STATE_BRIDGE_FAILED);
		}
                return ICD_STDERR;
            }
            /* Setup buffering information */
            memset(&bi, 0, sizeof(bi));
            bi.bufsize = CONF_SIZE / 2;
            bi.txbufpolicy = ZT_POLICY_IMMEDIATE;
            bi.rxbufpolicy = ZT_POLICY_IMMEDIATE;
            bi.numbufs = 4;
            if (ioctl(fd, ZT_SET_BUFINFO, &bi)) {
                cw_log(LOG_WARNING, "Unable to set buffering information: %s\n", strerror(errno));
                close(fd);
        	if(icd_caller__has_flag(that, ICD_MONITOR_FLAG)){
	     		icd_caller__set_state(that, ICD_CALLER_STATE_CALL_END);
		}
		else {
	     		icd_caller__set_state(that, ICD_CALLER_STATE_BRIDGE_FAILED);
		}
                return ICD_STDERR;
            }

            if (ioctl(fd, ZT_SETLINEAR, &x)) {
                cw_log(LOG_WARNING, "Unable to set linear mode: %s\n", strerror(errno));
                close(fd);
        	if(icd_caller__has_flag(that, ICD_MONITOR_FLAG)){
	     		icd_caller__set_state(that, ICD_CALLER_STATE_CALL_END);
		}
		else {
	     		icd_caller__set_state(that, ICD_CALLER_STATE_BRIDGE_FAILED);
		}
                return ICD_STDERR;
            }
            nfds = 1;
        } else {
            /* XXX Make sure we're not running on a pseudo channel XXX */
            fd = chan->fds[0];
            nfds = 0;

        }
    }
    /* in case they are in any other conference, unlink it */
    wipe_conference(fd);
    icd_caller__stop_waiting(that);
    /* apply the conference flags and create the link */

    confno = conf->ztc.confno;
    if (icd_caller__has_flag(that, ICD_MONITOR_FLAG))
        flags = CONF_MODE_MUTE;

    else
        flags = that->conf_mode ? that->conf_mode : CONF_MODE_REGULAR;

    activate_conference(fd, confno, flags);
    conf->usecount++;

    if (!icd_caller__has_flag(that, ICD_MONITOR_FLAG) && icd_caller__has_flag(that, ICD_CONF_MEMBER_FLAG))
        conf_play(that->conference, ENTER);
//PF Once more linear format
    /* Set it into linear mode (write) */
    if (cw_set_write_format(chan, icd_conf_format) < 0) {
        cw_log(LOG_WARNING, "Unable to set '%s' to write correct audio codec mode[%d]\n", chan->name,
            icd_conf_format);
        if (pseudo_fd) close(fd);
        	if(icd_caller__has_flag(that, ICD_MONITOR_FLAG)){
	     		icd_caller__set_state(that, ICD_CALLER_STATE_CALL_END);
		}
		else {
	     		icd_caller__set_state(that, ICD_CALLER_STATE_BRIDGE_FAILED);
		}
        return ICD_STDERR;
    }

    /* Set it into linear mode (read) */
    if (cw_set_read_format(chan, icd_conf_format) < 0) {
        cw_log(LOG_WARNING, "Unable to set '%s' to read correct audio codec mode[%d]\n", chan->name,
            icd_conf_format);
        if (pseudo_fd) close(fd);
        	if(icd_caller__has_flag(that, ICD_MONITOR_FLAG)){
	     		icd_caller__set_state(that, ICD_CALLER_STATE_CALL_END);
		}
		else {
	     		icd_caller__set_state(that, ICD_CALLER_STATE_BRIDGE_FAILED);
		}
        return ICD_STDERR;
    }
	
    for (;;) {
        outfd = -1;
        ms = -1;
        read_frame = NULL;

        active_channel = cw_waitfor_nandfds(&chan, 1, &fd, nfds, NULL, &outfd, &ms);

//        if (icd_caller__has_flag(that, ICD_CONF_MEMBER_FLAG)) {
//            if (!that || (that->state != ICD_CALLER_STATE_CONFERENCED) || !that->conference)
//                break;
//        }
        /* reasons to exit the conference */
	/* owner is no more in the conference */
        if (!that || (that->state != ICD_CALLER_STATE_CONFERENCED) || !that->conference
            || !that->conference->owner || (that->conference->owner->state != ICD_CALLER_STATE_CONFERENCED)
            ){
            break;
	}    
        if (active_channel) {
            read_frame = cw_read(active_channel);
            if (!read_frame)
                break;

/* This part of code (if) is for ZAP channels to make possible for muxmon to record 2 channels */
         if(!strcmp(chan->type, "DAHDI")){
	    if (chan->spiers && (read_frame->frametype == CW_FRAME_VOICE) && 
	                        (read_frame->subclass == icd_conf_format)) {
			struct cw_channel_spy *spying;
			for (spying = chan->spiers; spying; spying=spying->next) {
			cw_queue_spy_frame(spying, read_frame, 1);
			}
	    }	
          }  
	    if ((read_frame->frametype == CW_FRAME_DTMF) && (read_frame->subclass == '*')) {   /* '*'=end conference */
                ret = 0;
                break;
            } else if ((read_frame->frametype == CW_FRAME_DTMF) && (read_frame->subclass == '#')) {    /* '#'=toggle mute */
                flags = (flags == CONF_MODE_MUTE) ? CONF_MODE_REGULAR : CONF_MODE_MUTE;
                activate_conference(fd, confno, flags);
            } else if ((read_frame->frametype == CW_FRAME_DTMF)) {     /* infomative echo */
                cw_log(LOG_NOTICE, "%d->[%c]\n", read_frame->frametype, read_frame->subclass);
            } else if (fd != chan->fds[0]) {
                if (read_frame->frametype == CW_FRAME_VOICE) {
                    if (read_frame->subclass == icd_conf_format) {
                        /* Carefully write */
                   if (icd_debug)
                        cw_log(LOG_DEBUG, "ICD Write voice frame from channel[%s] to fd[%d] len[%d]\n", active_channel->name, fd, read_frame->datalen); 
			  
                        careful_write(fd, read_frame->data, read_frame->datalen);
                    } else
                        cw_log(LOG_WARNING, "Huh?  Got a non-linear (%d) frame in the conference\n",
                            read_frame->subclass);
                }
            }
            cw_fr_free(read_frame);
            read_frame = NULL;
        } else if (outfd > -1) {
            res = read(outfd, buf, CONF_SIZE);
            if (res > 0)
            {
                cw_fr_init_ex(&write_frame, CW_FRAME_VOICE, icd_conf_format, NULL);
                write_frame.datalen = res;
                write_frame.samples = res;
                write_frame.data = buf;
                write_frame.offset = CW_FRIENDLY_OFFSET;
                if (cw_write(chan, &write_frame) < 0)
                {
                    cw_log(LOG_WARNING, "Unable to write frame to channel: %s\n", strerror(errno));
                    /* break; */
                }
            }
            else
            {
                cw_log(LOG_WARNING, "Failed to read frame: %s\n", strerror(errno));
            }
        }

    }
    if (read_frame) {
        cw_fr_free(read_frame);
        read_frame = NULL;
    }

    if (!icd_caller__has_flag(that, ICD_MONITOR_FLAG) && icd_caller__has_flag(that, ICD_CONF_MEMBER_FLAG))
        conf_play(that->conference, LEAVE);

    /* unhook the chan from this conference */
    wipe_conference(fd);
    conf->usecount--;
    if (conf->usecount < 0)
        conf->usecount = 0;

    icd_caller__set_state(that, ICD_CALLER_STATE_CALL_END);

    if (that->conference){
    	if(that->conference->owner == that){
/* We are conference owner - wait for all other users to end conference, the owner leaves as the last.
   This is to avoid cross removing of assosciacions (owner and user at the same time)      		
   It is enough to test conf->usecount probably */
            int tstep;
            int maxsteps = 100;  
            for (tstep=0; tstep < maxsteps; tstep++){
                if(!icd_caller_list__has_callers(icd_caller__get_associations(that))){
                	break;
                }
            	usleep(100000);
            }
            if(tstep >= maxsteps){
                cw_log(LOG_WARNING, "This is not supposed to happen. Conference owner name[%s] callerid[%s] leaves before members waiting over 10s for members lo leave.\n", 
                icd_caller__get_name(that), icd_caller__get_caller_id(that));
            	icd_caller__set_state_on_associations(that, ICD_CALLER_STATE_CALL_END);
            	usleep(100000);
                icd_caller__remove_all_associations(that);
            } 	
    	}
    	else {
/* ToDo In case of many conf users - only if last user leaves (not including owner) */   		
    		if (!icd_caller__has_flag(that, ICD_MONITOR_FLAG)){
            	icd_caller__set_state(that->conference->owner, ICD_CALLER_STATE_CALL_END);
    		}
    		icd_conference__clear(that);
            icd_caller__remove_all_associations(that);
    	}
    }
    if (pseudo_fd) close(fd);
       
    return ICD_SUCCESS;
}

icd_caller *icd_conference__get_owner(icd_conference * conf)
{
    return conf ? conf->owner : NULL;
}

void icd_conference__lock(icd_conference * conf, char *pin)
{
    if (conf && pin)
        strncpy(conf->pin, pin, sizeof(conf->pin));
}

char *icd_conference__key(icd_conference * conf)
{
    return (strlen(conf->pin)) ? conf->pin : NULL;
}

/* For Emacs:
 * Local Variables:
 * indent-tabs-mode:nil
 * tab-width:4
 * c-basic-offset:4
 * End:
 * For VIM:
 * vim:set softtabstop=4 shiftwidth=4 tabstop=4 expandtab:
 */

