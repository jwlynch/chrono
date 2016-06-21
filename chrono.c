/* chrono: terminal chronometer using ncurses, 2016-04 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/time.h>
#include <pthread.h>
#include <curses.h>

WINDOW *window;
struct timeval tv0, tvc, tvd, tvp0, tvp1, tvpd;
unsigned int days, hours, minutes, seconds;
unsigned int paused, pause_start, pause_time;
unsigned int pause_days, pause_hours, pause_minutes, pause_seconds;
unsigned int show_help, show_help_once;
int cx_prev, cy_prev;
char chrono_str[128];
pthread_t thr01;
pthread_attr_t thr_attr;

void chrono_help (void) {
	printf ("Help: Press h for this help message, "
		"space or p to pause, q or Ctrl^C to quit\n");
}

void tvdiff (struct timeval *tv0, struct timeval *tv1, struct timeval *tvdiff) {
	tvdiff->tv_sec = tv1->tv_sec - tv0->tv_sec;
	tvdiff->tv_usec = 1000000 - tv0->tv_usec;
	tvdiff->tv_usec += tv1->tv_usec;
	if (tvdiff->tv_usec > 1000000) {
		unsigned int extrasec = tvdiff->tv_usec / 1000000;
		tvdiff->tv_sec += extrasec;
		tvdiff->tv_usec -= extrasec * 1000000;
	}
}

void *thr01_func (void *arg) {
	gettimeofday (&tv0, NULL);
	while (1) {
		if (paused) { usleep (10000); continue; }
		gettimeofday (&tvc, NULL);
		if (!show_help_once) {
			if ((tvc.tv_sec - tv0.tv_sec) < 5) {
				cx_prev = getcurx (window);
				cy_prev = getcury (window);
				mvcur (cy_prev, cx_prev, 2, 2);
				chrono_help ();
				mvcur (2, getcurx (window), cy_prev, cx_prev);
				fflush (stdout);
			}
			else {
				show_help_once = 1;
				cx_prev = getcurx (window);
				cy_prev = getcury (window);
				mvcur (cy_prev, cx_prev, 2, 2);
				int cnt;
				for (cnt = 0; cnt < getmaxx(window); cnt++)
					fputc (' ', stdout);
				mvcur (2, getmaxx(window)-1, cy_prev, cx_prev);
				fflush (stdout);
			}
		}
		tvdiff (&tv0, &tvc, &tvd);
		seconds = tvd.tv_sec;
		if (pause_time) seconds -= pause_time;
		days = seconds / 60 / 60 / 24;
		if (days)
			seconds -= days * 60 * 60 * 24;
		hours = seconds / 60 / 60;
		if (hours)
			seconds -= hours * 60 * 60;
		minutes = seconds / 60;
		if (minutes)
			seconds -= minutes * 60;
		if (days)
			sprintf (chrono_str, "\r%d day%s %02d:%02d:%02d:%03d", days, (days > 1)?"s":"", 
				hours, minutes, seconds, (int)tvd.tv_usec / 1000);
		else
			sprintf (chrono_str, "\r%02d:%02d:%02d:%03d", hours, minutes, seconds, (int)tvd.tv_usec / 1000);
		printf ("%s", chrono_str);
		fflush (stdout);
		usleep (10000);
	}
	return NULL;
}

void thr01_start (void) {
	pthread_attr_init (&thr_attr);
	pthread_attr_setdetachstate (&thr_attr, PTHREAD_CREATE_DETACHED);

	pthread_create (&thr01, &thr_attr, thr01_func, NULL);
	pthread_detach (thr01);

	pthread_attr_destroy (&thr_attr);
}

void chrono_exit (void) {
	endwin ();
	printf ("%s\n", chrono_str);
}

int main (int argc, char **argv) {
	if (argc > 1) {
		if (strcmp(argv[1],"--help")==0 ||
			strcmp(argv[1],"-h")==0)
			chrono_help ();
	}

	window = initscr ();
	noecho ();

	atexit (chrono_exit);


	thr01_start ();

	int c;
	while (1) {
		c = getch ();
		if (c == 'H' || c == 'h') {
			show_help = !show_help;
			mvcur (0, 0, 2, 2);
			if (show_help)
				chrono_help ();
			else {
				int cnt;
				for (cnt = 0; cnt < getmaxx (window); cnt++)
					fputc (' ', stdout);
				fflush (stdout);
			}
			mvcur (2, 2, 0, 0);
		}
		else if (c == ' ' || c == 'P' || c == 'p') {
			paused = !paused;
			if (paused) {
				gettimeofday (&tvp0, NULL);
				mvcur (0, 0, 0, 16);
				printf ("paused");
				fflush (stdout);
			}
			else {
				gettimeofday (&tvp1, NULL);
				tvdiff (&tvp0, &tvp1, &tvpd);
				pause_time += tvpd.tv_sec;
				pause_days = pause_time / 60 / 60 / 24;
				pause_hours = (pause_time - (pause_days*24*60*60)) / 60 / 60;
				pause_minutes = (pause_time - (pause_days*24*60*60) - (pause_hours*60*60)) / 60;
				pause_seconds = (pause_time - (pause_days*24*60*60) - (pause_hours*60*60) - (pause_minutes*60));
				printf ("%s             \n\r%03u days %02u:%02u:%02u:%03ld paused", 
					chrono_str, pause_days, pause_hours, pause_minutes, pause_seconds, tvpd.tv_usec/1000);
				mvcur (1, getcurx(window), 0, strlen(chrono_str));
				fflush (stdout);
			}
		}
		else if (c == 'Q' || c == 'q') exit (0);
	}
	
	return 0;
}

