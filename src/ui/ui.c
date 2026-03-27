/*
 * ui.c:
 *
 */

#include <sys/types.h>

#include <ctype.h>
#include <curses.h>
#include <string.h>
#include <math.h>
#include <pthread.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include <netdb.h>

#include <pwd.h>

#include "../include/addr_hash.h"
#include "../include/serv_hash.h"
#include "../include/iftop.h"
#include "../include/resolver.h"
#include "../include/sorted_list.h"
#include "../include/options.h"
#include "../include/screenfilter.h"
#include "../include/host_pair_line.h"
#include "../include/procinfo.h"

#define HELP_TIME 2

#define HELP_MESSAGE                                                              \
    "Host display:                          General:\n"                           \
    " n - toggle DNS host resolution         P - pause display\n"                 \
    " s - toggle show source host            h - toggle this help display\n"      \
    " d - toggle show destination host       b - toggle bar graph display\n"      \
    " t - cycle line display mode            B - cycle bar graph average\n"       \
    "                                        T - toggle cumulative line totals\n" \
    "Port display:                           j/k - scroll display\n"              \
    " N - toggle service resolution          f - edit filter code\n"              \
    " S - toggle show source port            l - set screen filter\n"             \
    " D - toggle show destination port       L - lin/log scales\n"                \
    " p - toggle port display                ! - shell command\n"                 \
    " Z - toggle process display             q - quit\n"                          \
    "Sorting:\n"                                                                  \
    " 1/2/3 - sort by 1st/2nd/3rd column\n"                                       \
    " < - sort by source name\n"                                                  \
    " > - sort by dest name\n"                                                    \
    " o - freeze current order\n"                                                 \
    "\n"                                                                          \
    "iftop, version " IFTOP_VERSION


/* 2, 10 and 40 seconds (in slots: seconds / RESOLUTION) */
int history_divs[HISTORY_DIVISIONS] = {2, 10, 40};

#define UNIT_DIVISIONS 4
char *unit_bits[UNIT_DIVISIONS] = {"b", "kb", "Mb", "Gb"};
char *unit_bytes[UNIT_DIVISIONS] = {"B", "kB", "MB", "GB"};

extern hash_type *history;
extern int history_pos;
extern int history_len;

extern options_t options;

void ui_finish();

hash_type *screen_hash;
serv_table *service_hash;
sorted_list_type screen_list;
host_pair_line totals;
int peaksent, peakrecv, peaktotal;

#define HELP_MSG_SIZE 80
int showhelphint = 0;
int persistenthelp = 0;
time_t helptimer = 0;
char helpmsg[HELP_MSG_SIZE];
int dontshowdisplay = 0;

#define COLOR_PAIR_SENT    1
#define COLOR_PAIR_RECV    2
#define COLOR_PAIR_BOTH    3
#define GREEN_FOREGROUND   4
#define RED_FOREGROUND     5
#define BLUE_FOREGROUND    6
#define YELLOW_FOREGROUND  7
#define MAGENTA_FOREGROUND 8
#define CYAN_FOREGROUND    9
#define BLACK_FOREGROUND   10
#define WHITE_FOREGROUND   11
#define BOLD               1
#define NONBOLD            0


int RECEIVE_BAR_COLOR[] = {GREEN_FOREGROUND, BOLD};
int SENT_BAR_COLOR[] = {BLUE_FOREGROUND, BOLD};
int BOTH_BAR_COLOR[] = {RED_FOREGROUND, BOLD};

int SCALE_BAR_COLOR[] = {BLUE_FOREGROUND, BOLD};
int SCALE_MARKERS_COLOR[] = {BLUE_FOREGROUND, BOLD};
int DL_UL_INDICATOR_COLOR[] = {BLUE_FOREGROUND, BOLD};
int HOST1_COLOR[] = {CYAN_FOREGROUND, BOLD};
int HOST2_COLOR[] = {MAGENTA_FOREGROUND, BOLD};
int TWO_SECOND_TRANSFER_COLUMN_COLOR[] = {BLUE_FOREGROUND, NONBOLD};
int TEN_SECOND_TRANSFER_COLUMN_COLOR[] = {CYAN_FOREGROUND, BOLD};
int FOURTY_SECOND_TRANSFER_COLUMN_COLOR[] = {BLUE_FOREGROUND, BOLD};

int BOTTOM_BAR_COLOR[] = {BLUE_FOREGROUND, BOLD};
int CUM_LABEL_COLOR[] = {YELLOW_FOREGROUND, NONBOLD};
int PEAK_LABEL_COLOR[] = {MAGENTA_FOREGROUND, BOLD};
int RATES_LABEL_COLOR[] = {MAGENTA_FOREGROUND, BOLD};

int TOTAL_LABEL_COLOR[] = {BLUE_FOREGROUND, BOLD};
int CUM_TRANSFER_COLUMN_COLOR[] = {YELLOW_FOREGROUND, BOLD};
int PEAK_TRANSFER_COLUMN_COLOR[] = {YELLOW_FOREGROUND, BOLD};
int PROC_NAME_COLOR[] = {GREEN_FOREGROUND, BOLD};

int convertColorToInt(char *color) {

    // convert to lowercase
    int i = 0;
    for (i = 0; color[i]; i++) {
        color[i] = tolower(color[i]);
    }
    if (strcmp(color, "green") == 0) {
        return 4;
    }
    if (strcmp(color, "red") == 0) {
        return 5;
    }
    if (strcmp(color, "blue") == 0) {
        return 6;
    }
    if (strcmp(color, "yellow") == 0) {
        return 7;
    }
    if (strcmp(color, "magenta") == 0) {
        return 8;
    }
    if (strcmp(color, "cyan") == 0) {
        return 9;
    }
    if (strcmp(color, "black") == 0) {
        return 10;
    }
    if (strcmp(color, "white") == 0) {
        return 11;
    }

    // invalid color
    return -1;
}

int convertBoldToInt(char *bold) {
    // convert to lowercase

    int i = 0;
    for (i = 0; bold[i]; i++) {
        bold[i] = tolower(bold[i]);
    }

    if (strcmp(bold, "bold") == 0) {
        return 1;
    }
    if (strcmp(bold, "nonbold") == 0) {
        return 0;
    }

    // default to nonbold if invalid or '-'
    return 0;
}

void eraseAndLoop(int signum);

void getColors() {

    const char *homedir;

    if ((homedir = getenv("HOME")) == NULL) {
        struct passwd *pw = getpwuid(getuid());
        if (pw == NULL) {
            return;
        }
        homedir = pw->pw_dir;
    }

    const char *filename = "/.iftopcolors";

    char absolutePath[1024];
    snprintf(absolutePath, sizeof(absolutePath), "%s%s", homedir, filename);

    if (access(absolutePath, F_OK) != -1) {
        FILE *fp;
        fp = fopen(absolutePath, "r");
        if (!fp) {
            return;
        }

        int ch;
        int lineCount = 0;
        do {
            ch = fgetc(fp);
            if (ch == '\n') {
                lineCount++;
            }
        } while (ch != EOF);

        if (lineCount > 0) {
            rewind(fp);

            // set up defaults
            char *downloadBarString = "RECEIVE_BAR_COLOR";
            char downloadBarColor[100] = "green";

            char *uploadBarString = "SENT_BAR_COLOR";
            char uploadBarColor[100] = "blue";

            char *bothBarString = "BOTH_BAR_COLOR";
            char bothBarColor[100] = "magenta";

            char *scaleBarString = "SCALE_BAR_COLOR";
            char scaleBarColor[100] = "magenta";

            char *scaleMarkerString = "SCALE_MARKERS_COLOR";
            char scaleMarkerColor[100] = "blue";

            char *dl_ul_String = "DL_UL_INDICATOR_COLOR";
            char dl_ul_Color[100] = "blue";

            char *host1String = "HOST1_COLOR";
            char host1Color[100] = "cyan";

            char *host2String = "HOST2_COLOR";
            char host2Color[100] = "magenta";

            char *twoSecondTransferColumnString = "TWO_SECOND_TRANSFER_COLUMN_COLOR";
            char twoSecondTransferColumnColor[100] = "blue";

            char *tenSecondTransferColumnString = "TEN_SECOND_TRANSFER_COLUMN_COLOR";
            char tenSecondTransferColumnColor[100] = "yellow";

            char *fourtySecondTransferColumnString = "FOURTY_SECOND_TRANSFER_COLUMN_COLOR";
            char fourtySecondTransferColumnColor[100] = "yellow";

            char *bottomBarString = "BOTTOM_BAR_COLOR";
            char bottomBarColor[100] = "blue";

            char *cumLabelString = "CUM_LABEL_COLOR";
            char cumLabelColor[100] = "yellow";

            char *peakLabelString = "PEAK_LABEL_COLOR";
            char peakLabelColor[100] = "yellow";

            char *ratesLabelString = "RATES_LABEL_COLOR";
            char ratesLabelColor[100] = "yellow";

            char *totalLabelString = "TOTAL_LABEL_COLOR";
            char totalLabelColor[100] = "blue";

            char *cumTransferColumnString = "CUM_TRANSFER_COLUMN_COLOR";
            char cumTransferColumnColor[100] = "yellow";

            char *peakTransferColumnString = "PEAK_TRANSFER_COLUMN_COLOR";
            char peakTransferColumnColor[100] = "yellow";

            char *procNameString = "PROC_NAME_COLOR";
            char procNameColor[100] = "green";

            int i = 0;
            for (i = 0; i < lineCount; i++) {
                char buffer[255];
                char boldBuffer[50];
                // parse out first field
                fscanf(fp, "%254s", buffer);

                if (buffer[0] == '#') {
                    // skip this comment line
                    fgets(buffer, 255, fp);
                    continue;
                }

                if (strcmp(downloadBarString, buffer) == 0) {
                    fscanf(fp, "%99s", downloadBarColor);
                    int colorInt = convertColorToInt(downloadBarColor);
                    if (colorInt != -1) {
                        RECEIVE_BAR_COLOR[0] = colorInt;
                    }
                    fscanf(fp, "%49s", boldBuffer);
                    RECEIVE_BAR_COLOR[1] = convertBoldToInt(boldBuffer);
                }

                if (strcmp(uploadBarString, buffer) == 0) {
                    fscanf(fp, "%99s", uploadBarColor);

                    int colorInt = convertColorToInt(uploadBarColor);
                    if (colorInt != -1) {
                        SENT_BAR_COLOR[0] = colorInt;
                    }
                    fscanf(fp, "%49s", boldBuffer);
                    SENT_BAR_COLOR[1] = convertBoldToInt(boldBuffer);
                }

                if (strcmp(bothBarString, buffer) == 0) {
                    fscanf(fp, "%99s", bothBarColor);
                    int colorInt = convertColorToInt(bothBarColor);
                    if (colorInt != -1) {
                        BOTH_BAR_COLOR[0] = colorInt;
                    }
                    fscanf(fp, "%49s", boldBuffer);
                    BOTH_BAR_COLOR[1] = convertBoldToInt(boldBuffer);
                }

                if (strcmp(scaleBarString, buffer) == 0) {
                    fscanf(fp, "%99s", scaleBarColor);
                    int colorInt = convertColorToInt(scaleBarColor);
                    if (colorInt != -1) {
                        SCALE_BAR_COLOR[0] = colorInt;
                    }
                    fscanf(fp, "%49s", boldBuffer);
                    SCALE_BAR_COLOR[1] = convertBoldToInt(boldBuffer);
                }

                if (strcmp(scaleMarkerString, buffer) == 0) {
                    fscanf(fp, "%99s", scaleMarkerColor);
                    int colorInt = convertColorToInt(scaleMarkerColor);
                    if (colorInt != -1) {
                        SCALE_MARKERS_COLOR[0] = colorInt;
                    }
                    fscanf(fp, "%49s", boldBuffer);
                    SCALE_MARKERS_COLOR[1] = convertBoldToInt(boldBuffer);
                }

                if (strcmp(dl_ul_String, buffer) == 0) {
                    fscanf(fp, "%99s", dl_ul_Color);
                    int colorInt = convertColorToInt(dl_ul_Color);
                    if (colorInt != -1) {
                        DL_UL_INDICATOR_COLOR[0] = colorInt;
                    }
                    fscanf(fp, "%49s", boldBuffer);
                    DL_UL_INDICATOR_COLOR[1] = convertBoldToInt(boldBuffer);
                }

                if (strcmp(host1String, buffer) == 0) {
                    fscanf(fp, "%99s", host1Color);

                    int colorInt = convertColorToInt(host1Color);
                    if (colorInt != -1) {
                        HOST1_COLOR[0] = colorInt;
                    }
                    fscanf(fp, "%49s", boldBuffer);
                    HOST1_COLOR[1] = convertBoldToInt(boldBuffer);
                }

                if (strcmp(host2String, buffer) == 0) {
                    fscanf(fp, "%99s", host2Color);
                    int colorInt = convertColorToInt(host2Color);
                    if (colorInt != -1) {
                        HOST2_COLOR[0] = colorInt;
                    }
                    fscanf(fp, "%49s", boldBuffer);
                    HOST2_COLOR[1] = convertBoldToInt(boldBuffer);
                }

                if (strcmp(twoSecondTransferColumnString, buffer) == 0) {
                    fscanf(fp, "%99s", twoSecondTransferColumnColor);
                    int colorInt = convertColorToInt(twoSecondTransferColumnColor);
                    if (colorInt != -1) {
                        TWO_SECOND_TRANSFER_COLUMN_COLOR[0] = colorInt;
                    }
                    fscanf(fp, "%49s", boldBuffer);
                    TWO_SECOND_TRANSFER_COLUMN_COLOR[1] = convertBoldToInt(boldBuffer);
                }
                if (strcmp(tenSecondTransferColumnString, buffer) == 0) {
                    fscanf(fp, "%99s", tenSecondTransferColumnColor);
                    int colorInt = convertColorToInt(tenSecondTransferColumnColor);
                    if (colorInt != -1) {
                        TEN_SECOND_TRANSFER_COLUMN_COLOR[0] = colorInt;
                    }
                    fscanf(fp, "%49s", boldBuffer);
                    TEN_SECOND_TRANSFER_COLUMN_COLOR[1] = convertBoldToInt(boldBuffer);
                }
                if (strcmp(fourtySecondTransferColumnString, buffer) == 0) {
                    fscanf(fp, "%99s", fourtySecondTransferColumnColor);
                    int colorInt = convertColorToInt(fourtySecondTransferColumnColor);
                    if (colorInt != -1) {
                        FOURTY_SECOND_TRANSFER_COLUMN_COLOR[0] = colorInt;
                    }
                    fscanf(fp, "%49s", boldBuffer);
                    FOURTY_SECOND_TRANSFER_COLUMN_COLOR[1] = convertBoldToInt(boldBuffer);
                }
                if (strcmp(bottomBarString, buffer) == 0) {
                    fscanf(fp, "%99s", bottomBarColor);
                    int colorInt = convertColorToInt(bottomBarColor);
                    if (colorInt != -1) {
                        BOTTOM_BAR_COLOR[0] = colorInt;
                    }
                    fscanf(fp, "%49s", boldBuffer);
                    BOTTOM_BAR_COLOR[1] = convertBoldToInt(boldBuffer);
                }
                if (strcmp(cumLabelString, buffer) == 0) {
                    fscanf(fp, "%99s", cumLabelColor);
                    int colorInt = convertColorToInt(cumLabelColor);
                    if (colorInt != -1) {
                        CUM_LABEL_COLOR[0] = colorInt;
                    }
                    fscanf(fp, "%49s", boldBuffer);
                    CUM_LABEL_COLOR[1] = convertBoldToInt(boldBuffer);
                }
                if (strcmp(peakLabelString, buffer) == 0) {
                    fscanf(fp, "%99s", peakLabelColor);
                    int colorInt = convertColorToInt(peakLabelColor);
                    if (colorInt != -1) {
                        PEAK_LABEL_COLOR[0] = colorInt;
                    }
                    fscanf(fp, "%49s", boldBuffer);
                    PEAK_LABEL_COLOR[1] = convertBoldToInt(boldBuffer);
                }
                if (strcmp(ratesLabelString, buffer) == 0) {
                    fscanf(fp, "%99s", ratesLabelColor);
                    int colorInt = convertColorToInt(ratesLabelColor);
                    if (colorInt != -1) {
                        RATES_LABEL_COLOR[0] = colorInt;
                    }
                    fscanf(fp, "%49s", boldBuffer);
                    RATES_LABEL_COLOR[1] = convertBoldToInt(boldBuffer);
                }
                if (strcmp(totalLabelString, buffer) == 0) {
                    fscanf(fp, "%99s", totalLabelColor);
                    int colorInt = convertColorToInt(totalLabelColor);
                    if (colorInt != -1) {
                        TOTAL_LABEL_COLOR[0] = colorInt;
                    }
                    fscanf(fp, "%49s", boldBuffer);
                    TOTAL_LABEL_COLOR[1] = convertBoldToInt(boldBuffer);
                }
                if (strcmp(cumTransferColumnString, buffer) == 0) {
                    fscanf(fp, "%99s", cumTransferColumnColor);
                    int colorInt = convertColorToInt(cumTransferColumnColor);
                    if (colorInt != -1) {
                        CUM_TRANSFER_COLUMN_COLOR[0] = colorInt;
                    }
                    fscanf(fp, "%49s", boldBuffer);
                    CUM_TRANSFER_COLUMN_COLOR[1] = convertBoldToInt(boldBuffer);
                }
                if (strcmp(peakTransferColumnString, buffer) == 0) {
                    fscanf(fp, "%99s", peakTransferColumnColor);
                    int colorInt = convertColorToInt(peakTransferColumnColor);
                    if (colorInt != -1) {
                        PEAK_TRANSFER_COLUMN_COLOR[0] = colorInt;
                    }
                    fscanf(fp, "%49s", boldBuffer);
                    PEAK_TRANSFER_COLUMN_COLOR[1] = convertBoldToInt(boldBuffer);
                }
                if (strcmp(procNameString, buffer) == 0) {
                    fscanf(fp, "%99s", procNameColor);
                    int colorInt = convertColorToInt(procNameColor);
                    if (colorInt != -1) {
                        PROC_NAME_COLOR[0] = colorInt;
                    }
                    fscanf(fp, "%49s", boldBuffer);
                    PROC_NAME_COLOR[1] = convertBoldToInt(boldBuffer);
                }
            }

        } else {
            /* empty file, use defaults */
        }

        fclose(fp);
    }
}

void ui_curses_init() {

    (void)initscr(); /* initialize the curses library */
    if (has_colors() == TRUE) {
        start_color();        /* Start color          */
        use_default_colors(); /* retain terminal fg , bg colors */
        // change colors for transfer and receive bars

        // init color for <= and =>
        init_pair(RED_FOREGROUND, COLOR_RED, -1);
        init_pair(BLUE_FOREGROUND, COLOR_BLUE, -1);
        init_pair(GREEN_FOREGROUND, COLOR_GREEN, -1);
        init_pair(YELLOW_FOREGROUND, COLOR_YELLOW, -1);
        init_pair(MAGENTA_FOREGROUND, COLOR_MAGENTA, -1);
        init_pair(CYAN_FOREGROUND, COLOR_CYAN, -1);
        init_pair(WHITE_FOREGROUND, COLOR_WHITE, -1);
        init_pair(BLACK_FOREGROUND, COLOR_BLACK, -1);

        getColors();

        signal(SIGQUIT, eraseAndLoop);
    }
    keypad(stdscr, TRUE); /* enable keyboard mapping */
    (void)nonl();         /* tell curses not to do NL->CR/NL on output */
    (void)cbreak();       /* take input chars one at a time, no wait for \n */
    (void)noecho();       /* don't echo input */
    halfdelay(1);
}


void turnOnColor(int color_ary[]) {

    int fg = color_ary[0];
    attron(COLOR_PAIR(fg));

    if (color_ary[1] == 1) {
        attron(A_BOLD);
    }
}

void turnOffColor(int color_ary[]) {
    int fg = color_ary[0];
    attroff(COLOR_PAIR(fg));

    if (color_ary[1] == 1) {
        attroff(A_BOLD);
    }
}


/*
 * Compare two screen lines based on bandwidth.  Start comparing from the
 * specified column
 */
int screen_line_bandwidth_compare(host_pair_line *aa, host_pair_line *bb, int start_div) {
    int i;
    switch (options.linedisplay) {
    case OPTION_LINEDISPLAY_ONE_LINE_SENT:
        for (i = start_div; i < HISTORY_DIVISIONS; i++) {
            if (aa->sent[i] != bb->sent[i]) {
                return (aa->sent[i] < bb->sent[i]);
            }
        }
        break;
    case OPTION_LINEDISPLAY_ONE_LINE_RECV:
        for (i = start_div; i < HISTORY_DIVISIONS; i++) {
            if (aa->recv[i] != bb->recv[i]) {
                return (aa->recv[i] < bb->recv[i]);
            }
        }
        break;
    case OPTION_LINEDISPLAY_TWO_LINE:
    case OPTION_LINEDISPLAY_ONE_LINE_BOTH:
        /* fallback to the combined sent+recv that also act as fallback for sent/recv */
        break;
    }
    for (i = start_div; i < HISTORY_DIVISIONS; i++) {
        if (aa->recv[i] + aa->sent[i] != bb->recv[i] + bb->sent[i]) {
            return (aa->recv[i] + aa->sent[i] < bb->recv[i] + bb->sent[i]);
        }
    }
    return 1;
}

/*
 * Compare two screen lines based on pre-resolved hostname.
 * Hostnames are cached in cached_hostname before sorting to avoid
 * repeated resolve() calls (which acquire a mutex) during qsort.
 */
int screen_line_host_compare(host_pair_line *aa, host_pair_line *bb) {
    int cmp_result = strcmp(aa->cached_hostname, bb->cached_hostname);

    if (cmp_result == 0) {
        return screen_line_bandwidth_compare(aa, bb, 2);
    } else {
        return (cmp_result > 0);
    }
}

int screen_line_compare(void *left, void *right) {
    host_pair_line *aa = (host_pair_line *)left;
    host_pair_line *bb = (host_pair_line *)right;
    if (options.sort == OPTION_SORT_DIV1) {
        return screen_line_bandwidth_compare(aa, bb, 0);
    } else if (options.sort == OPTION_SORT_DIV2) {
        return screen_line_bandwidth_compare(aa, bb, 1);
    } else if (options.sort == OPTION_SORT_DIV3) {
        return screen_line_bandwidth_compare(aa, bb, 2);
    } else if (options.sort == OPTION_SORT_SRC || options.sort == OPTION_SORT_DEST) {
        return screen_line_host_compare(aa, bb);
    }

    return 1;
}

void readable_size(float bandwidth, char *buf, int bsize, int ksize, int bytes) {

    int i = 0;
    float size = 1;

    /* Convert to bits? */
    if (bytes == 0) {
        bandwidth *= 8;
    }

    while (1) {
        if (bandwidth < size * 1000 || i >= UNIT_DIVISIONS - 1) {
            snprintf(buf, bsize, " %4.0f%s", bandwidth / size,
                     bytes ? unit_bytes[i] : unit_bits[i]);
            break;
        }
        i++;
        size *= ksize;
        if (bandwidth < size * 10) {
            snprintf(buf, bsize, " %4.2f%s", bandwidth / size,
                     bytes ? unit_bytes[i] : unit_bits[i]);
            break;
        } else if (bandwidth < size * 100) {
            snprintf(buf, bsize, " %4.1f%s", bandwidth / size,
                     bytes ? unit_bytes[i] : unit_bits[i]);
            break;
        }
    }
}


/* Barchart scales. */
static struct {
    int max, interval;
} scale[] = {
    {64000, 10},                                        /* 64 kbit/s */
    {128000, 10},   {256000, 10},     {1000000, 10},    /* 1 Mbit/s */
    {10000000, 10}, {100000000, 100}, {1000000000, 100} /* 1 Gbit/s */
};
static int rateidx = 0, wantbiggerrate;

static int rateidx_init = 0;

static int get_bar_interval(float bandwidth) {
    int i = 10;
    if (bandwidth > 100000000) {
        i = 100;
    }
    return i;
}

static float get_max_bandwidth() {
    float max;
    if (options.max_bandwidth > 0) {
        max = options.max_bandwidth;
    } else {
        max = scale[rateidx].max;
    }
    return max;
}

/* rate in bits — returns length in 1/8th character units for sub-cell precision */
static int get_bar_length_eighths(const int rate) {
    float bar_length;
    if (rate <= 0) {
        return 0;
    }
    if (rate > scale[rateidx].max) {
        wantbiggerrate = 1;
        if (!rateidx_init) {
            while (rate > scale[rateidx_init++].max) {}
            rateidx = rateidx_init;
        }
    }
    if (options.log_scale) {
        bar_length = log(rate) / log(get_max_bandwidth());
    } else {
        bar_length = rate / get_max_bandwidth();
    }
    return (int)(bar_length * COLS * 8);
}

static int get_bar_length(const int rate) {
    return get_bar_length_eighths(rate) / 8;
}

static void draw_bar_scale(int *row) {
    float marker;
    float max, interval;
    max = get_max_bandwidth();
    interval = get_bar_interval(max);
    if (options.showbars) {
        float stop;
        /* Draw bar graph scale on top of the window. */
        move(*row, 0);
        clrtoeol();

        mvhline(*row + 1, 0, 0, COLS);
        /* marker in bytes */

        if (options.log_scale) {
            marker = 1.25;
            stop = max / 8;
        } else {
            marker = max / (5 * 8);
            stop = max / 8;
        }

        /* for (marker = 1.25; marker * 8 <= max; marker *= interval) { */
        while (marker <= stop) {
            char size_buf[40], *label;
            int xpos;
            /* This 1024 vs 1000 stuff is just plain evil */
            readable_size(marker, size_buf, sizeof size_buf, options.log_scale ? 1000 : 1024,
                          options.bandwidth_in_bytes);
            label = size_buf + strspn(size_buf, " ");
            xpos = get_bar_length(marker * 8);
            mvaddch(*row + 1, xpos, ACS_BTEE);
            if (xpos + strlen(label) >= COLS) {
                xpos = COLS - strlen(label);
            }

            turnOnColor(SCALE_MARKERS_COLOR);

            mvaddstr(*row, xpos, label);

            turnOffColor(SCALE_MARKERS_COLOR);


            if (options.log_scale) {
                marker *= interval;
            } else {
                marker += max / (5 * 8);
            }
        }
        mvaddch(*row + 1, 0, ACS_LLCORNER);
        *row += 2;
    } else {
        mvhline(*row, 0, 0, COLS);
        *row += 1;
    }
}

extern double slot_elapsed;

double history_length(const int division) {
    int full_slots;
    if (history_len < history_divs[division]) {
        full_slots = history_len;
    } else {
        full_slots = history_divs[division];
    }
    /* The newest slot is only partially filled — use elapsed fraction
     * instead of a full RESOLUTION to avoid rate dips after rotation. */
    if (full_slots <= 1) {
        return slot_elapsed > 0 ? slot_elapsed : RESOLUTION;
    }
    return (full_slots - 1) * RESOLUTION + slot_elapsed;
}

void draw_line_total(float sent, float recv, int row, int col, option_linedisplay_t linedisplay,
                     int bytes) {
    char buf[10];
    float bandwidth;
    switch (linedisplay) {
    case OPTION_LINEDISPLAY_TWO_LINE:
        draw_line_total(sent, recv, row, col, OPTION_LINEDISPLAY_ONE_LINE_SENT, bytes);
        draw_line_total(sent, recv, row + 1, col, OPTION_LINEDISPLAY_ONE_LINE_RECV, bytes);
        break;
    case OPTION_LINEDISPLAY_ONE_LINE_SENT:
        bandwidth = sent;
        break;
    case OPTION_LINEDISPLAY_ONE_LINE_RECV:
        bandwidth = recv;
        break;
    case OPTION_LINEDISPLAY_ONE_LINE_BOTH:
        bandwidth = recv + sent;
        break;
    }


    if (linedisplay != OPTION_LINEDISPLAY_TWO_LINE) {
        readable_size(bandwidth, buf, 10, 1024, bytes);
        if (bandwidth == sent) {
            turnOnColor(TWO_SECOND_TRANSFER_COLUMN_COLOR);
        } else if (bandwidth == recv) {
            turnOnColor(TEN_SECOND_TRANSFER_COLUMN_COLOR);
        } else {
            turnOnColor(FOURTY_SECOND_TRANSFER_COLUMN_COLOR);
        }
        mvaddstr(row, col, buf);

        if (bandwidth == sent) {
            turnOffColor(TWO_SECOND_TRANSFER_COLUMN_COLOR);
        } else if (bandwidth == recv) {
            turnOffColor(TEN_SECOND_TRANSFER_COLUMN_COLOR);
        } else {
            turnOffColor(FOURTY_SECOND_TRANSFER_COLUMN_COLOR);
        }
    }
}

#define BAR_SMOOTH_MAX_ROWS 256
#define BAR_SMOOTH_FACTOR 0.15f  /* smaller = smoother glide */
static float bar_smooth[BAR_SMOOTH_MAX_ROWS];

void draw_bar(float bandwidth, int row, short colorpair) {
    int length;
    float target;
    colorpair = has_colors() == TRUE ? colorpair : 0;
    mvchgat(row, 0, 0, A_NORMAL, 0, NULL);
    target = get_bar_length_eighths(8 * bandwidth) / 8.0f;
    if (row >= 0 && row < BAR_SMOOTH_MAX_ROWS) {
        float prev = bar_smooth[row];
        if (prev < 0.5f && target < 0.5f) {
            bar_smooth[row] = 0;
        } else {
            bar_smooth[row] = prev + (target - prev) * BAR_SMOOTH_FACTOR;
        }
        length = (int)(bar_smooth[row] + 0.5f);
    } else {
        length = (int)(target + 0.5f);
    }
    if (length > 0) {
        if (length > COLS) length = COLS;
        mvchgat(row, 0, length, A_REVERSE, colorpair, NULL);
    }
}

void draw_line_totals(int row, host_pair_line *line, option_linedisplay_t linedisplay) {
    int j;
    int x = (COLS - 8 * HISTORY_DIVISIONS);

    for (j = 0; j < HISTORY_DIVISIONS; j++) {
        draw_line_total(line->sent[j], line->recv[j], row, x, linedisplay,
                        options.bandwidth_in_bytes);
        x += 8;
    }

    if (options.showbars) {
        switch (linedisplay) {
        case OPTION_LINEDISPLAY_TWO_LINE:
            if (SENT_BAR_COLOR[1] == BOLD) {
                attron(A_BOLD);
            }
            draw_bar(line->sent[options.bar_interval], row, SENT_BAR_COLOR[0]);
            if (SENT_BAR_COLOR[1] == BOLD) {
                attroff(A_BOLD);
            }
            if (RECEIVE_BAR_COLOR[1] == BOLD) {
                attron(A_BOLD);
            }
            draw_bar(line->recv[options.bar_interval], row + 1, RECEIVE_BAR_COLOR[0]);

            if (RECEIVE_BAR_COLOR[1] == BOLD) {
                attroff(A_BOLD);
            }
            break;
        case OPTION_LINEDISPLAY_ONE_LINE_SENT:
            if (SENT_BAR_COLOR[1] == BOLD) {
                attron(A_BOLD);
            }
            draw_bar(line->sent[options.bar_interval], row, SENT_BAR_COLOR[0]);
            if (SENT_BAR_COLOR[1] == BOLD) {
                attroff(A_BOLD);
            }
            break;
        case OPTION_LINEDISPLAY_ONE_LINE_RECV:
            if (RECEIVE_BAR_COLOR[1] == BOLD) {
                attron(A_BOLD);
            }
            draw_bar(line->recv[options.bar_interval], row, RECEIVE_BAR_COLOR[0]);
            if (RECEIVE_BAR_COLOR[1] == BOLD) {
                attroff(A_BOLD);
            }
            break;
        case OPTION_LINEDISPLAY_ONE_LINE_BOTH:
            if (BOTH_BAR_COLOR[1] == BOLD) {
                attron(A_BOLD);
            }
            draw_bar(line->recv[options.bar_interval] + line->sent[options.bar_interval], row,
                     BOTH_BAR_COLOR[0]);
            if (BOTH_BAR_COLOR[1] == BOLD) {
                attroff(A_BOLD);
            }
            break;
        }
    }
}

void draw_totals(host_pair_line *totals) {
    /* Draw rule */
    int y = LINES - 4;
    int j;
    char buf[10];
    int x = (COLS - 8 * HISTORY_DIVISIONS);
    y++;
    draw_line_totals(y, totals, OPTION_LINEDISPLAY_TWO_LINE);
    y += 2;
    for (j = 0; j < HISTORY_DIVISIONS; j++) {
        readable_size((totals->sent[j] + totals->recv[j]), buf, 10, 1024,
                      options.bandwidth_in_bytes);
        if (j == 0) {
            turnOnColor(TOTAL_LABEL_COLOR);
        } else if (j == 1) {
            turnOnColor(TOTAL_LABEL_COLOR);
        } else {
            turnOnColor(TOTAL_LABEL_COLOR);
        }
        mvaddstr(y, x, buf);
        if (j == 0) {
            turnOffColor(TOTAL_LABEL_COLOR);
        } else if (j == 1) {
            turnOffColor(TOTAL_LABEL_COLOR);
        } else {
            turnOffColor(TOTAL_LABEL_COLOR);
        }
        x += 8;
    }
}

extern history_type history_totals;

void screen_list_init() {
    screen_list.compare = &screen_line_compare;
    sorted_list_initialise(&screen_list);
}

void screen_list_clear() {
    peaksent = peakrecv = peaktotal = 0;
    sorted_list_destroy(&screen_list);
}

void calculate_totals(const int hist_idx[]) {
    int i;
    long recv_bytes, sent_bytes;

    /* Peaks: scan all history */
    for (i = 0; i < HISTORY_LENGTH; i++) {
        recv_bytes = history_totals.recv[i];
        sent_bytes = history_totals.sent[i];
        if (recv_bytes > peakrecv) {
            peakrecv = recv_bytes;
        }
        if (sent_bytes > peaksent) {
            peaksent = sent_bytes;
        }
        if (recv_bytes + sent_bytes > peaktotal) {
            peaktotal = recv_bytes + sent_bytes;
        }
    }

    /* Accumulate totals across history divisions */
    for (i = 0; i < history_divs[0]; i++) {
        recv_bytes = history_totals.recv[hist_idx[i]];
        sent_bytes = history_totals.sent[hist_idx[i]];
        totals.recv[0] += recv_bytes;
        totals.sent[0] += sent_bytes;
        totals.recv[1] += recv_bytes;
        totals.sent[1] += sent_bytes;
        totals.recv[2] += recv_bytes;
        totals.sent[2] += sent_bytes;
    }
    for (; i < history_divs[1]; i++) {
        recv_bytes = history_totals.recv[hist_idx[i]];
        sent_bytes = history_totals.sent[hist_idx[i]];
        totals.recv[1] += recv_bytes;
        totals.sent[1] += sent_bytes;
        totals.recv[2] += recv_bytes;
        totals.sent[2] += sent_bytes;
    }
    for (; i < HISTORY_LENGTH; i++) {
        totals.recv[2] += history_totals.recv[hist_idx[i]];
        totals.sent[2] += history_totals.sent[hist_idx[i]];
    }

    /* Divide by time spans */
    for (i = 0; i < HISTORY_DIVISIONS; i++) {
        double hl = history_length(i);
        if (hl > 0) {
            totals.recv[i] /= hl;
            totals.sent[i] /= hl;
        }
    }
}

void make_screen_list() {
    hash_node_type *node = NULL;
    int i;
    int count = 0;
    int capacity = 256;
    void **items = NULL;
    double inv_hist_len[HISTORY_DIVISIONS];
    const int freeze = options.freezeorder;

    for (i = 0; i < HISTORY_DIVISIONS; i++) {
        double hl = history_length(i);
        inv_hist_len[i] = hl > 0 ? 1.0 / hl : 0;
    }

    if (!freeze) {
        items = xmalloc(capacity * sizeof(void *));
    }

    while (hash_next_item(screen_hash, &node) == HASH_STATUS_OK) {
        host_pair_line *line = (host_pair_line *)node->record;
        for (i = 0; i < HISTORY_DIVISIONS; i++) {
            line->recv[i] *= inv_hist_len[i];
            line->sent[i] *= inv_hist_len[i];
        }

        if (!freeze) {
            if (count >= capacity) {
                capacity *= 2;
                void *tmp = realloc(items, capacity * sizeof(void *));
                if (!tmp) {
                    break;
                }
                items = tmp;
            }
            items[count++] = line;
        }
    }

    if (!freeze) {
        /* Pre-resolve hostnames before sorting to avoid repeated
         * resolve() calls (each acquiring a mutex) during qsort. */
        if (options.sort == OPTION_SORT_SRC || options.sort == OPTION_SORT_DEST) {
            int is_src = (options.sort == OPTION_SORT_SRC);
            for (i = 0; i < count; i++) {
                host_pair_line *hpl = (host_pair_line *)items[i];
                void *addr = is_src ? (void *)&hpl->ap.src6 : (void *)&hpl->ap.dst6;
                if (options.dnsresolution) {
                    resolve(hpl->ap.address_family, addr, hpl->cached_hostname, HOSTNAME_LENGTH);
                } else {
                    inet_ntop(hpl->ap.address_family, addr, hpl->cached_hostname,
                              sizeof(hpl->cached_hostname));
                }
            }
        }

        sorted_list_insert_batch(&screen_list, items, count);
        free(items);
    }
}

/*
 * Zeros all data in the screen hash, but does not remove items.
 */
void screen_hash_clear() {
    hash_node_type *node = NULL;
    while (hash_next_item(screen_hash, &node) == HASH_STATUS_OK) {
        host_pair_line *hpl = (host_pair_line *)node->record;
        hpl->total_recv = hpl->total_sent = 0;
        memset(hpl->recv, 0, sizeof(hpl->recv));
        memset(hpl->sent, 0, sizeof(hpl->sent));
    }
}

void analyse_data() {
    hash_node_type *node = NULL;
    int hist_idx[HISTORY_LENGTH];
    int i;

    if (options.paused == 1) {
        return;
    }

    /* Precompute history index array once */
    for (i = 0; i < HISTORY_LENGTH; i++) {
        hist_idx[i] = (HISTORY_LENGTH + history_pos - i) % HISTORY_LENGTH;
    }

    // Zero totals
    memset(&totals, 0, sizeof totals);

    if (options.freezeorder) {
        screen_hash_clear();
    } else {
        screen_list_clear();
        addr_hash_delete_all_free(screen_hash);
    }

    /* Cache aggregation options outside the loop */
    const int agg_src = options.aggregate_src;
    const int agg_dst = options.aggregate_dest;
    const int showports = options.showports;

    while (hash_next_item(history, &node) == HASH_STATUS_OK) {
        history_type *hist_data = (history_type *)node->record;
        host_pair_line *screen_line;
        union {
            host_pair_line **h_p_l_pp;
            void **void_pp;
        } u_screen_line = {&screen_line};
        addr_pair ap;

        ap = *(addr_pair *)node->key;

        /* Aggregate hosts, if required */
        if (agg_src) {
            memset(&ap.src6, '\0', sizeof(ap.src6));
        }
        if (agg_dst) {
            memset(&ap.dst6, '\0', sizeof(ap.dst6));
        }

        /* Aggregate ports, if required */
        if (showports == OPTION_PORTS_DEST || showports == OPTION_PORTS_OFF) {
            ap.src_port = 0;
        }
        if (showports == OPTION_PORTS_SRC || showports == OPTION_PORTS_OFF) {
            ap.dst_port = 0;
        }
        if (showports == OPTION_PORTS_OFF) {
            ap.protocol = 0;
        }


        if (addr_hash_find(screen_hash, &ap, u_screen_line.void_pp) == HASH_STATUS_KEY_NOT_FOUND) {
            screen_line = xcalloc(1, sizeof *screen_line);
            addr_hash_insert(screen_hash, &ap, screen_line);
            screen_line->ap = ap;
        }

        /* Resolve owning process if screen_line doesn't have one yet.
         * Check history cache first (persists after socket closes),
         * then try a live lookup on both sides of the flow. */
        if (options.show_processes && !screen_line->process_name[0]) {
            if (hist_data->proc_resolved) {
                screen_line->pid = hist_data->proc_pid;
                strncpy(screen_line->process_name, hist_data->proc_name, PROCNAME_LENGTH - 1);
                screen_line->process_name[PROCNAME_LENGTH - 1] = '\0';
            } else {
                addr_pair *orig = (addr_pair *)node->key;
                proc_entry pe;
                int found = 0;
                if (orig->address_family == AF_INET) {
                    found = procinfo_lookup(AF_INET, &orig->src, orig->src_port, orig->protocol, &pe);
                    if (!found)
                        found = procinfo_lookup(AF_INET, &orig->dst, orig->dst_port, orig->protocol, &pe);
                } else if (orig->address_family == AF_INET6) {
                    found = procinfo_lookup(AF_INET6, &orig->src6, orig->src_port, orig->protocol, &pe);
                    if (!found)
                        found = procinfo_lookup(AF_INET6, &orig->dst6, orig->dst_port, orig->protocol, &pe);
                }
                if (found) {
                    screen_line->pid = pe.pid;
                    strncpy(screen_line->process_name, pe.name, PROCNAME_LENGTH - 1);
                    screen_line->process_name[PROCNAME_LENGTH - 1] = '\0';
                    hist_data->proc_resolved = 1;
                    hist_data->proc_pid = pe.pid;
                    strncpy(hist_data->proc_name, pe.name, PROCINFO_NAME_MAX - 1);
                    hist_data->proc_name[PROCINFO_NAME_MAX - 1] = '\0';
                }
            }
        }

        screen_line->total_sent += hist_data->total_sent;
        screen_line->total_recv += hist_data->total_recv;

        /* Accumulate history slots into division buckets.
         * Slot i contributes to div j if i < history_divs[j]. */
        {
            long recv_bytes, sent_bytes;
            /* Slots 0..divs[0]-1: contribute to all 3 divisions */
            for (i = 0; i < history_divs[0]; i++) {
                recv_bytes = hist_data->recv[hist_idx[i]];
                sent_bytes = hist_data->sent[hist_idx[i]];
                screen_line->recv[0] += recv_bytes;
                screen_line->sent[0] += sent_bytes;
                screen_line->recv[1] += recv_bytes;
                screen_line->sent[1] += sent_bytes;
                screen_line->recv[2] += recv_bytes;
                screen_line->sent[2] += sent_bytes;
            }
            /* Slots divs[0]..divs[1]-1: contribute to div 1 and 2 */
            for (; i < history_divs[1]; i++) {
                recv_bytes = hist_data->recv[hist_idx[i]];
                sent_bytes = hist_data->sent[hist_idx[i]];
                screen_line->recv[1] += recv_bytes;
                screen_line->sent[1] += sent_bytes;
                screen_line->recv[2] += recv_bytes;
                screen_line->sent[2] += sent_bytes;
            }
            /* Slots divs[1]..HISTORY_LENGTH-1: contribute to div 2 only */
            for (; i < HISTORY_LENGTH; i++) {
                screen_line->recv[2] += hist_data->recv[hist_idx[i]];
                screen_line->sent[2] += hist_data->sent[hist_idx[i]];
            }
        }
    }

    make_screen_list();


    calculate_totals(hist_idx);
}

void sprint_host(char *line, int af, struct in6_addr *addr, unsigned int port,
                 unsigned int protocol, int maxlen) {
    char hostname[HOSTNAME_LENGTH];
    char service[HOSTNAME_LENGTH];
    const char *s_name;
    int left;

    if (IN6_IS_ADDR_UNSPECIFIED(addr)) {
        snprintf(hostname, sizeof(hostname), " * ");
    } else {
        if (options.dnsresolution) {
            resolve(af, addr, hostname, maxlen);
        } else {
            inet_ntop(af, addr, hostname, sizeof(hostname));
        }
    }
    left = strlen(hostname);

    if (port != 0) {
        s_name = (options.portresolution) ? serv_table_lookup(service_hash, port, protocol) : NULL;
        if (s_name) {
            snprintf(service, HOSTNAME_LENGTH, ":%s", s_name);
        } else {
            snprintf(service, HOSTNAME_LENGTH, ":%d", port);
        }
    } else {
        service[0] = '\0';
    }


    snprintf(line, maxlen + 1, "%-*s", maxlen, hostname);
    if (left > (maxlen - (int)strlen(service))) {
        left = maxlen - strlen(service);
        if (left < 0) {
            left = 0;
        }
    }
    snprintf(line + left, maxlen - left + 1, "%-*s", maxlen - left, service);
}


void ui_print() {
    sorted_list_node *nn = NULL;
    char host1[HOSTNAME_LENGTH], host2[HOSTNAME_LENGTH];
    static char *line;
    static int lcols;
    int y = 0;

    if (dontshowdisplay) {
        return;
    }

    if (!line || lcols != COLS) {
        xfree(line);
        line = calloc(COLS + 1, 1);
        lcols = COLS;
    }

    /*
     * erase() is faster than clear().  Dunno why we switched to
     * clear()
     */
    erase();

    turnOnColor(SCALE_BAR_COLOR);
    draw_bar_scale(&y);

    turnOffColor(SCALE_BAR_COLOR);

    if (options.showhelp) {
        mvaddstr(y, 0, HELP_MESSAGE);
    } else {
        int i = 0;

        while (i < options.screen_offset &&
               ((nn = sorted_list_next_item(&screen_list, nn)) != NULL)) {
            i++;
        }

        /* Screen layout: we have 2 * HISTORY_DIVISIONS 6-character wide history
         * items, and so can use COLS - 12 * HISTORY_DIVISIONS to print the two
         * host names. */

        if (i == 0 || nn != NULL) {
            while ((y < LINES - 5) && ((nn = sorted_list_next_item(&screen_list, nn)) != NULL)) {

                int L;


                host_pair_line *screen_line = (host_pair_line *)nn->data;

                if (y < LINES - 5) {
                    L = (COLS - 8 * HISTORY_DIVISIONS - 4) / 2;
                    if (options.show_totals) {
                        L -= 4;
                    }
                    if (options.show_processes) {
                        L -= 8; /* reserve space for [procname] */
                    }
                    if (L > HOSTNAME_LENGTH) {
                        L = HOSTNAME_LENGTH;
                    }
                    if (L < 1) L = 1;

                    sprint_host(host1, screen_line->ap.address_family, &(screen_line->ap.src6),
                                screen_line->ap.src_port, screen_line->ap.protocol, L);
                    sprint_host(host2, screen_line->ap.address_family, &(screen_line->ap.dst6),
                                screen_line->ap.dst_port, screen_line->ap.protocol, L);

                    if (!screen_filter_match(host1) && !screen_filter_match(host2)) {
                        continue;
                    }
                    int x = 0;
                    turnOnColor(HOST1_COLOR);

                    if (L < 1) {
                        x = 1;
                    }

                    mvaddstr(y, x, host1);
                    turnOffColor(HOST1_COLOR);

                    if (L < 1 && x == 1) {}

                    x += L;


                    turnOnColor(DL_UL_INDICATOR_COLOR);

                    switch (options.linedisplay) {
                    case OPTION_LINEDISPLAY_TWO_LINE:
                        mvaddstr(y, x, " => ");
                        mvaddstr(y + 1, x, " <= ");
                        break;
                    case OPTION_LINEDISPLAY_ONE_LINE_BOTH:
                        mvaddstr(y, x, "<=> ");
                        break;
                    case OPTION_LINEDISPLAY_ONE_LINE_SENT:
                        mvaddstr(y, x, " => ");
                        break;
                    case OPTION_LINEDISPLAY_ONE_LINE_RECV:
                        mvaddstr(y, x, " <= ");
                        break;
                    }

                    turnOffColor(DL_UL_INDICATOR_COLOR);

                    x += 4;


                    turnOnColor(HOST2_COLOR);
                    mvaddstr(y, x, host2);
                    turnOffColor(HOST2_COLOR);

                    if (options.show_processes && screen_line->process_name[0]) {
                        char proc_label[PROCNAME_LENGTH + 16];
                        snprintf(proc_label, sizeof(proc_label), " [%d:%.12s]",
                                 screen_line->pid, screen_line->process_name);
                        turnOnColor(PROC_NAME_COLOR);
                        mvaddstr(y, x + L, proc_label);
                        turnOffColor(PROC_NAME_COLOR);
                    }

                    if (options.show_totals) {
                        draw_line_total(screen_line->total_sent, screen_line->total_recv, y,
                                        COLS - 8 * (HISTORY_DIVISIONS + 1), options.linedisplay, 1);
                    }

                    draw_line_totals(y, screen_line, options.linedisplay);
                }
                if (options.linedisplay == OPTION_LINEDISPLAY_TWO_LINE) {
                    y += 2;
                } else {
                    y += 1;
                }
            }
        }
    }


    y = LINES - 3;


    turnOnColor(BOTTOM_BAR_COLOR);

    mvhline(y - 1, 0, 0, COLS);

    turnOffColor(BOTTOM_BAR_COLOR);

    mvaddstr(y, 0, "TX: ");
    mvaddstr(y + 1, 0, "RX: ");
    turnOnColor(TOTAL_LABEL_COLOR);
    mvaddstr(y + 2, 0, "TOTAL: ");

    turnOffColor(TOTAL_LABEL_COLOR);

    /* Cummulative totals */
    turnOnColor(CUM_LABEL_COLOR);
    mvaddstr(y, 16, "cum: ");
    turnOffColor(CUM_LABEL_COLOR);

    readable_size(history_totals.total_sent, line, 10, 1024, 1);
    turnOnColor(CUM_TRANSFER_COLUMN_COLOR);
    mvaddstr(y, 22, line);

    readable_size(history_totals.total_recv, line, 10, 1024, 1);

    mvaddstr(y + 1, 22, line);

    readable_size(history_totals.total_recv + history_totals.total_sent, line, 10, 1024, 1);
    mvaddstr(y + 2, 22, line);
    turnOffColor(CUM_TRANSFER_COLUMN_COLOR);


    /* peak traffic */
    turnOnColor(PEAK_LABEL_COLOR);
    mvaddstr(y, 32, "peak: ");
    turnOffColor(PEAK_LABEL_COLOR);

    turnOnColor(PEAK_TRANSFER_COLUMN_COLOR);

    readable_size((float)peaksent / RESOLUTION, line, 10, 1024, options.bandwidth_in_bytes);
    mvaddstr(y, 39, line);

    readable_size((float)peakrecv / RESOLUTION, line, 10, 1024, options.bandwidth_in_bytes);
    mvaddstr(y + 1, 39, line);

    readable_size((float)peaktotal / RESOLUTION, line, 10, 1024, options.bandwidth_in_bytes);
    mvaddstr(y + 2, 39, line);
    turnOffColor(PEAK_TRANSFER_COLUMN_COLOR);


    turnOnColor(RATES_LABEL_COLOR);

    mvaddstr(y, COLS - 8 * HISTORY_DIVISIONS - 8, "rates:");
    turnOffColor(RATES_LABEL_COLOR);

    draw_totals(&totals);


    if (showhelphint) {
        mvaddstr(0, 0, " ");
        mvaddstr(0, 1, helpmsg);
        mvaddstr(0, 1 + strlen(helpmsg), " ");
        mvchgat(0, 0, strlen(helpmsg) + 2, A_REVERSE, 0, NULL);
    }

    move(LINES - 1, COLS - 1);

    refresh();

    /* Bar chart auto scale */
    if (wantbiggerrate && options.max_bandwidth == 0) {
        ++rateidx;
        wantbiggerrate = 0;
    }
}

void ui_tick(int print) {
    if (print) {
        ui_print();
    } else if (showhelphint && (time(NULL) - helptimer > HELP_TIME) && !persistenthelp) {
        showhelphint = 0;
        ui_print();
    }
}


void showhelp(const char *message) {
    strncpy(helpmsg, message, HELP_MSG_SIZE);
    showhelphint = 1;
    helptimer = time(NULL);
    persistenthelp = 0;
    tick(1);
}

void ui_init() {
    char msg[20];
    ui_curses_init();

    erase();

    screen_list_init();
    screen_hash = addr_hash_create();

    service_hash = serv_table_create();
    serv_table_init(service_hash);

    snprintf(msg, 20, "Listening on %s", options.interface);
    showhelp(msg);
}


void showportstatus() {
    if (options.showports == OPTION_PORTS_ON) {
        showhelp("Port display ON");
    } else if (options.showports == OPTION_PORTS_OFF) {
        showhelp("Port display OFF");
    } else if (options.showports == OPTION_PORTS_DEST) {
        showhelp("Port display DEST");
    } else if (options.showports == OPTION_PORTS_SRC) {
        showhelp("Port display SOURCE");
    }
}


void ui_loop() {
    /* in edline.c */

    char *edline(int linenum, const char *prompt, const char *initial);
    /* in iftop.c */
    char *set_filter_code(const char *filter);

    extern sig_atomic_t foad;

    while (foad == 0) {
        int keycode;
        keycode = getch();
        switch (keycode) {
        case 'q':
            foad = 1;
            break;

        case 'n':
            if (options.dnsresolution) {
                options.dnsresolution = 0;
                showhelp("DNS resolution off");
            } else {
                options.dnsresolution = 1;
                showhelp("DNS resolution on");
            }
            tick(1);
            break;

        case 'N':
            if (options.portresolution) {
                options.portresolution = 0;
                showhelp("Port resolution off");
            } else {
                options.portresolution = 1;
                showhelp("Port resolution on");
            }
            tick(1);
            break;

        case 'h':
        case '?':
            options.showhelp = !options.showhelp;
            tick(1);
            break;

        case 'b':
            if (options.showbars) {
                options.showbars = 0;
                showhelp("Bars off");
            } else {
                options.showbars = 1;
                showhelp("Bars on");
            }
            tick(1);
            break;

        case 'B':
            options.bar_interval = (options.bar_interval + 1) % 3;
            if (options.bar_interval == 0) {
                showhelp("Bars show 2s average");
            } else if (options.bar_interval == 1) {
                showhelp("Bars show 10s average");
            } else {
                showhelp("Bars show 40s average");
            }
            ui_print();
            break;
        case 's':
            if (options.aggregate_src) {
                options.aggregate_src = 0;
                showhelp("Show source host");
            } else {
                options.aggregate_src = 1;
                showhelp("Hide source host");
            }
            break;
        case 'd':
            if (options.aggregate_dest) {
                options.aggregate_dest = 0;
                showhelp("Show dest host");
            } else {
                options.aggregate_dest = 1;
                showhelp("Hide dest host");
            }
            break;
        case 'S':
            /* Show source ports */
            if (options.showports == OPTION_PORTS_OFF) {
                options.showports = OPTION_PORTS_SRC;
            } else if (options.showports == OPTION_PORTS_DEST) {
                options.showports = OPTION_PORTS_ON;
            } else if (options.showports == OPTION_PORTS_ON) {
                options.showports = OPTION_PORTS_DEST;
            } else {
                options.showports = OPTION_PORTS_OFF;
            }
            showportstatus();
            break;
        case 'D':
            /* Show dest ports */
            if (options.showports == OPTION_PORTS_OFF) {
                options.showports = OPTION_PORTS_DEST;
            } else if (options.showports == OPTION_PORTS_SRC) {
                options.showports = OPTION_PORTS_ON;
            } else if (options.showports == OPTION_PORTS_ON) {
                options.showports = OPTION_PORTS_SRC;
            } else {
                options.showports = OPTION_PORTS_OFF;
            }
            showportstatus();
            break;
        case 'p':
            options.showports =
                (options.showports == OPTION_PORTS_OFF) ? OPTION_PORTS_ON : OPTION_PORTS_OFF;
            showportstatus();
            // Don't tick here, otherwise we get a bogus display
            break;
        case 'P':
            if (options.paused) {
                options.paused = 0;
                showhelp("Display unpaused");
            } else {
                options.paused = 1;
                showhelp("Display paused");
                persistenthelp = 1;
            }
            break;
        case 'o':
            if (options.freezeorder) {
                options.freezeorder = 0;
                showhelp("Order unfrozen");
            } else {
                options.freezeorder = 1;
                showhelp("Order frozen");
                persistenthelp = 1;
            }
            break;
        case '1':
            options.sort = OPTION_SORT_DIV1;
            showhelp("Sort by col 1");
            break;
        case '2':
            options.sort = OPTION_SORT_DIV2;
            showhelp("Sort by col 2");
            break;
        case '3':
            options.sort = OPTION_SORT_DIV3;
            showhelp("Sort by col 3");
            break;
        case '<':
            options.sort = OPTION_SORT_SRC;
            showhelp("Sort by source");
            break;
        case '>':
            options.sort = OPTION_SORT_DEST;
            showhelp("Sort by dest");
            break;
        case 'j':
            options.screen_offset++;
            ui_print();
            break;
        case 'k':
            if (options.screen_offset > 0) {
                options.screen_offset--;
                ui_print();
            }
            break;
        case 't':
            options.linedisplay = (options.linedisplay + 1) % 4;
            switch (options.linedisplay) {
            case OPTION_LINEDISPLAY_TWO_LINE:
                showhelp("Two lines per host");
                break;
            case OPTION_LINEDISPLAY_ONE_LINE_SENT:
                showhelp("Sent traffic only");
                break;
            case OPTION_LINEDISPLAY_ONE_LINE_RECV:
                showhelp("Received traffic only");
                break;
            case OPTION_LINEDISPLAY_ONE_LINE_BOTH:
                showhelp("One line per host");
                break;
            }
            ui_print();
            break;
        case 'f': {
            char *s;
            dontshowdisplay = 1;
            if ((s = edline(0, "Net filter", options.filtercode))) {
                char *m;
                if (s[strspn(s, " \t")] == 0) {
                    /* Empty filter; set to NULL. */
                    xfree(s);
                    s = NULL;
                }
                if (!(m = set_filter_code(s))) {
                    xfree(options.filtercode);
                    options.filtercode = s;
                    /* -lpcap will write junk to stderr; we do our best to
                     * erase it.... */
                    move(COLS - 1, LINES - 1);
                    wrefresh(curscr);
                    showhelp("Installed new filter");
                } else {
                    showhelp(m);
                    xfree(s);
                }
            }
            dontshowdisplay = 0;
            ui_print();
            break;
        }
        case 'l': {
#ifdef HAVE_REGCOMP
            char *s;
            dontshowdisplay = 1;
            if ((s = edline(0, "Screen filter", options.screenfilter))) {
                if (!screen_filter_set(s)) {
                    showhelp("Invalid regexp");
                }
            }
            dontshowdisplay = 0;
            ui_print();
#else
            showhelp("Sorry, screen filters not supported on this platform")
#endif
            break;
        }
        case '!': {
#ifdef ALLOW_SUBSHELL
            char *s;
            dontshowdisplay = 1;
            if ((s = edline(0, "Command", "")) && s[strspn(s, " \t")]) {
                int exit_status, needs_wait = 0;
                erase();
                refresh();
                endwin();
                errno = 0;
                exit_status = system(s);
                if (exit_status == -1 || (exit_status == 127 && errno != 0)) {
                    fprintf(stderr, "system: %s: %s\n", s, strerror(errno));
                    needs_wait = 1;
                } else if (exit_status != 0) {
                    if (WIFEXITED(exit_status)) {
                        fprintf(stderr, "%s: exited with code %d\n", s, WEXITSTATUS(exit_status));
                    } else if (WIFSIGNALED(exit_status)) {
                        fprintf(stderr, "%s: killed by signal %d\n", s, WTERMSIG(exit_status));
                    }
                    needs_wait = 1;
                }
                ui_curses_init();
                if (needs_wait) {
                    fprintf(stderr, "Press any key....");
                    while (getch() == ERR)
                        ;
                }
                erase();
                xfree(s);
            }
            dontshowdisplay = 0;
#else
            showhelp("Sorry, subshells have been disabled.");
#endif
            break;
        }
        case 'T':
            options.show_totals = !options.show_totals;
            if (options.show_totals) {
                showhelp("Show cumulative totals");
            } else {
                showhelp("Hide cumulative totals");
            }
            ui_print();
            break;
        case 'L':
            options.log_scale = !options.log_scale;
            showhelp(options.log_scale ? "Logarithmic scale" : "Linear scale");
            ui_print();
            break;
        case 'Z':
            options.show_processes = !options.show_processes;
            showhelp(options.show_processes ? "Process display on" : "Process display off");
            tick(1);
            break;
        case KEY_CLEAR:
        case 12: /* ^L */
            wrefresh(curscr);
            break;
        case ERR:
            break;
        default:
            showhelp("Press H or ? for help");
            break;
        }
        tick(0);
    }
}

void ui_finish() {
    endwin();
}

void eraseAndLoop(int signum) {
    getColors();
    erase();

    ui_loop();
}
