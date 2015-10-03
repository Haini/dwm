/* Modified example source from suckless.org */
#define _BSD_SOURCE
#include <unistd.h>
#include <stdio.h>

#include <alsa/asoundlib.h>
#ifndef _STRUCT_TIMESPEC
#	define _STRUCT_TIMESPEC
#endif
#ifndef _STRUCT_TIMEVAL
#	define _STRUCT_TIMEVAL
#endif

#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <strings.h>




//#include <sys/time.h>
//#include <time.h> 



#include <sys/types.h>
#include <sys/wait.h>
#include <inttypes.h>


#include <alsa/control.h>

#include <errno.h>

#include <sys/statvfs.h>
#include <sys/sysinfo.h>
#include <sys/utsname.h>

#include <X11/Xlib.h>


#define BATT_NOW        "/sys/class/power_supply/BAT0/charge_now"
#define BATT_FULL       "/sys/class/power_supply/BAT0/charge_full"
#define BATT_STATUS       "/sys/class/power_supply/BAT0/status"

#define MB 1048576
#define GB 1073741824

#define MAXLEN 24

char *tzargentina = "America/Buenos_Aires";
char *tzutc = "UTC";
char *tzberlin = "Europe/Berlin";

static Display *dpy;

char *
smprintf(char *fmt, ...)
{
	va_list fmtargs;
	char *ret;
	int len;

	va_start(fmtargs, fmt);
	len = vsnprintf(NULL, 0, fmt, fmtargs);
	va_end(fmtargs);

	ret = malloc(++len);
	
    if (ret == NULL) {
		perror("malloc");
		exit(1);
	}

	va_start(fmtargs, fmt);
	vsnprintf(ret, len, fmt, fmtargs);
	va_end(fmtargs);

	return ret;
}

void
settz(char *tzname)
{
	setenv("TZ", tzname, 1);
}

char *
mktimes(char *fmt, char *tzname)
{
	char buf[129];
	time_t tim;
	struct tm *timtm;

	memset(buf, 0, sizeof(buf));
	settz(tzname);
	tim = time(NULL);
	timtm = localtime(&tim);
	if (timtm == NULL) {
		perror("localtime");
		exit(1);
	}

	if (!strftime(buf, sizeof(buf)-1, fmt, timtm)) {
		fprintf(stderr, "strftime == 0\n");
		exit(1);
	}

	return smprintf("%s", buf);
}

char *
get_battery(){
    long lnum1, lnum2 = 0;
    char *status = malloc(sizeof(char)*12);
    char s = '?';
    FILE *fp = NULL;
    if ((fp = fopen(BATT_NOW, "r"))) {
        fscanf(fp, "%ld\n", &lnum1);
        fclose(fp);
        fp = fopen(BATT_FULL, "r");
        fscanf(fp, "%ld\n", &lnum2);
        fclose(fp);
        fp = fopen(BATT_STATUS, "r");
        fscanf(fp, "%s\n", status);
        fclose(fp);
        if (strcmp(status,"Charging") == 0)
            s = '+';
        if (strcmp(status,"Discharging") == 0)
            s = '-';
        if (strcmp(status,"Full") == 0)
            s = '=';

        free(status);

        return smprintf("%c%ld%%", s,(lnum1/(lnum2/100)));
    }
        else return smprintf("");
}

char * 
get_ram()
{
    uintmax_t used = 0, total = 0, percent = 0;
    
    struct sysinfo mem;
    sysinfo(&mem);

    total   = (uintmax_t) mem.totalram / MB;
    used    = (uintmax_t) (mem.totalram - mem.freeram -
    mem.bufferram - mem.sharedram) / MB;
    percent = (used * 100) / total;
  
    return smprintf(" %dMB / %dMB", used, total);
    
//    char buf[129];

//    memcpy(buf, (char*) percent, sizeof(percent));

//    return smprintf("%s", buf);

//    return total;
}


void
setstatus(char *str)
{
	XStoreName(dpy, DefaultRootWindow(dpy), str);
	XSync(dpy, False);
}

char *
loadavg(void)
{
	double avgs[3];

	if (getloadavg(avgs, 3) < 0) {
		perror("getloadavg");
		exit(1);
	}

	return smprintf("%.2f %.2f %.2f", avgs[0], avgs[1], avgs[2]);
}

char* get_volume()
{
    snd_mixer_t *handle;
    snd_mixer_elem_t *elem;
    snd_mixer_selem_id_t *s_elem;

    snd_mixer_open(&handle, 0);
    snd_mixer_attach(handle, "default");
    snd_mixer_selem_register(handle, NULL, NULL);
    snd_mixer_load(handle);
    snd_mixer_selem_id_malloc(&s_elem);
    snd_mixer_selem_id_set_name(s_elem, "Master");

    elem = snd_mixer_find_selem(handle, s_elem);

    if (NULL == elem)
    {
        snd_mixer_selem_id_free(s_elem);
        snd_mixer_close(handle);

        exit(EXIT_FAILURE);
    }

    long int vol, max, min, percent;

    snd_mixer_selem_get_playback_volume_range(elem, &min, &max);
    snd_mixer_selem_get_playback_volume(elem, 0, &vol);
    
    int test;
    snd_mixer_selem_get_playback_switch(elem, 0, &test);
   
    percent = (vol * 100) / max;

    snd_mixer_selem_id_free(s_elem);
    snd_mixer_close(handle);
   
    if(test <= 0) {
        return smprintf("V[%c]", 'M');
    }

    return smprintf("V[%d]", percent);
}


int
main(void)
{
	char *status;
	char *avgs;
	char *tmutc;
    char *battery;
    char *ram;
    char *vol;

	if (!(dpy = XOpenDisplay(NULL))) {
		fprintf(stderr, "dwmstatus: cannot open display.\n");
		return 1;
	}

	for (;;sleep(1)) {
		avgs = loadavg();
		tmutc = mktimes("%H:%M", tzberlin);
        ram = get_ram();
        battery = get_battery();
        vol = get_volume();
		
        status = smprintf("\x03[RAM:%s] \x04 [BATTERY%s] \x06 %s \x07| %s |",
				 ram, battery, vol, tmutc);
		
        setstatus(status);
		free(avgs);
		free(tmutc);
		free(status);
        free(battery);
        free(ram);
	}

	XCloseDisplay(dpy);

	return 0;
}

