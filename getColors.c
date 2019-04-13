//
// Created by Jacob Menke on 6/16/17.
//

#include <stdio.h>
#include <sys/types.h>
#include <pwd.h>
#include <unistd.h>
#include <stdlib.h>
#include <memory.h>
#include <ctype.h>
#include "getColors.h"

int convertColorToInt(char *color) {

    //convert to lowercase
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

    //invalid color
    return -1;

}

int convertBoldToInt(char *bold) {
    //convert to lowercase
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

    //default to nonbold if invalid or '-'
    return 0;


}

void getColors() {

    const char *homedir;

    if ((homedir = getenv("HOME")) == NULL) {
        homedir = getpwuid(getuid())->pw_dir;
    }

    const char *filename = "/.iftopcolors";
    char absolutePath[200];
    strcat(absolutePath, homedir);
    strcat(absolutePath, filename);

    if (access(absolutePath, F_OK) != -1) {
        FILE *fp;
        fp = fopen(absolutePath, "r");

        int ch;
        int lineCount = 0;
        do {
            ch = fgetc(fp);
            if (ch == '\n') lineCount++;
        } while (ch != EOF);

        if (lineCount > 0) {
            rewind(fp);

            //set up defaults
            char *downloadBarString = "RECEIVE_BAR_COLOR";
            char downloadBarColor[100] = "green";

            char *uploadBarString = "SENT_BAR_COLOR";
            char uploadBarColor[100] = "blue";

            char *bothBarString = "BOTH_BAR_COLOR";
            char bothBarColor[100] = "blue";

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


            int i = 0;
            for (i = 0; i < lineCount; i++) {
                char buffer[255];
                char boldBuffer[50];
                fscanf(fp, "%s", buffer);
                if (buffer[0] == '#') {
                    //skip this comment line
                    fgets(buffer, 255, fp);
                    continue;

                }


                if (strcmp(downloadBarString, buffer) == 0) {
                    fscanf(fp, "%s", downloadBarColor);
                    int colorInt = convertColorToInt(downloadBarColor);
                    if (colorInt != -1) {
                        RECEIVE_BAR_COLOR[0] = colorInt;
                    }
                    fscanf(fp, "%s", boldBuffer);
                    RECEIVE_BAR_COLOR[1] = convertBoldToInt(boldBuffer);


                }

                if (strcmp(uploadBarString, buffer) == 0) {
                    fscanf(fp, "%s", uploadBarColor);

                    int colorInt = convertColorToInt(uploadBarColor);
                    if (colorInt != -1) {
                        SENT_BAR_COLOR[0] = colorInt;
                    }
                    fscanf(fp, "%s", boldBuffer);
                    SENT_BAR_COLOR[1] = convertBoldToInt(boldBuffer);

                }

                if (strcmp(bothBarString, buffer) == 0) {
                    fscanf(fp, "%s", bothBarColor);
                    int colorInt = convertColorToInt(bothBarColor);
                    if (colorInt != -1) {
                        BOTH_BAR_COLOR[0] = colorInt;
                    }
                    fscanf(fp, "%s", boldBuffer);
                    BOTH_BAR_COLOR[1] = convertBoldToInt(boldBuffer);

                }

                if (strcmp(scaleBarString, buffer) == 0) {
                    fscanf(fp, "%s", scaleBarColor);
                    int colorInt = convertColorToInt(scaleBarColor);
                    if (colorInt != -1) {
                        SCALE_BAR_COLOR[0] = colorInt;
                    }
                    fscanf(fp, "%s", boldBuffer);
                    SCALE_BAR_COLOR[1] = convertBoldToInt(boldBuffer);

                }


                if (strcmp(scaleMarkerString, buffer) == 0) {
                    fscanf(fp, "%s", scaleMarkerColor);
                    int colorInt = convertColorToInt(scaleMarkerColor);
                    if (colorInt != -1) {
                        SCALE_MARKERS_COLOR[0] = colorInt;
                    }
                    fscanf(fp, "%s", boldBuffer);
                    SCALE_MARKERS_COLOR[1] = convertBoldToInt(boldBuffer);

                }

                if (strcmp(dl_ul_String, buffer) == 0) {
                    fscanf(fp, "%s", dl_ul_Color);
                    int colorInt = convertColorToInt(dl_ul_Color);
                    if (colorInt != -1) {
                        DL_UL_INDICATOR_COLOR[0] = colorInt;
                    }
                    fscanf(fp, "%s", boldBuffer);
                    DL_UL_INDICATOR_COLOR[1] = convertBoldToInt(boldBuffer);

                }

                if (strcmp(host1String, buffer) == 0) {
                    fscanf(fp, "%s", host1Color);

                    int colorInt = convertColorToInt(host1Color);
                    if (colorInt != -1) {
                        HOST1_COLOR[0] = colorInt;
                    }
                    fscanf(fp, "%s", boldBuffer);
                    HOST1_COLOR[1] = convertBoldToInt(boldBuffer);
                }

                if (strcmp(host2String, buffer) == 0) {
                    fscanf(fp, "%s", host2Color);
                    int colorInt = convertColorToInt(host2Color);
                    if (colorInt != -1) {
                        HOST2_COLOR[0] = colorInt;
                    }
                    fscanf(fp, "%s", boldBuffer);
                    HOST2_COLOR[1] = convertBoldToInt(boldBuffer);
                }

                if (strcmp(twoSecondTransferColumnString, buffer) == 0) {
                    fscanf(fp, "%s", twoSecondTransferColumnColor);
                    int colorInt = convertColorToInt(twoSecondTransferColumnColor);
                    if (colorInt != -1) {
                        TWO_SECOND_TRANSFER_COLUMN_COLOR[0] = colorInt;
                    }
                    fscanf(fp, "%s", boldBuffer);
                    TWO_SECOND_TRANSFER_COLUMN_COLOR[1] = convertBoldToInt(boldBuffer);

                }
                if (strcmp(tenSecondTransferColumnString, buffer) == 0) {
                    fscanf(fp, "%s", tenSecondTransferColumnColor);
                    int colorInt = convertColorToInt(tenSecondTransferColumnColor);
                    if (colorInt != -1) {
                        TEN_SECOND_TRANSFER_COLUMN_COLOR[0] = colorInt;
                    }
                    fscanf(fp, "%s", boldBuffer);
                    TEN_SECOND_TRANSFER_COLUMN_COLOR[1] = convertBoldToInt(boldBuffer);
                }
                if (strcmp(fourtySecondTransferColumnString, buffer) == 0) {
                    fscanf(fp, "%s", fourtySecondTransferColumnColor);
                    int colorInt = convertColorToInt(fourtySecondTransferColumnColor);
                    if (colorInt != -1) {
                        FOURTY_SECOND_TRANSFER_COLUMN_COLOR[0] = colorInt;
                    }
                    fscanf(fp, "%s", boldBuffer);
                    FOURTY_SECOND_TRANSFER_COLUMN_COLOR[1] = convertBoldToInt(boldBuffer);
                }
                if (strcmp(bottomBarString, buffer) == 0) {
                    fscanf(fp, "%s", bottomBarColor);
                    int colorInt = convertColorToInt(bottomBarColor);
                    if (colorInt != -1) {
                        BOTTOM_BAR_COLOR[0] = colorInt;
                    }
                    fscanf(fp, "%s", boldBuffer);
                    BOTTOM_BAR_COLOR[1] = convertBoldToInt(boldBuffer);
                }
                if (strcmp(cumLabelString, buffer) == 0) {
                    fscanf(fp, "%s", cumLabelColor);
                    int colorInt = convertColorToInt(cumLabelColor);
                    if (colorInt != -1) {
                        CUM_LABEL_COLOR[0] = colorInt;
                    }
                    fscanf(fp, "%s", boldBuffer);
                    CUM_LABEL_COLOR[1] = convertBoldToInt(boldBuffer);
                }
                if (strcmp(peakLabelString, buffer) == 0) {
                    fscanf(fp, "%s", peakLabelColor);
                    int colorInt = convertColorToInt(peakLabelColor);
                    if (colorInt != -1) {
                        PEAK_LABEL_COLOR[0] = colorInt;
                    }
                    fscanf(fp, "%s", boldBuffer);
                    PEAK_LABEL_COLOR[1] = convertBoldToInt(boldBuffer);
                }
                if (strcmp(ratesLabelString, buffer) == 0) {
                    fscanf(fp, "%s", ratesLabelColor);
                    int colorInt = convertColorToInt(ratesLabelColor);
                    if (colorInt != -1) {
                        RATES_LABEL_COLOR[0] = colorInt;
                    }
                    fscanf(fp, "%s", boldBuffer);
                    RATES_LABEL_COLOR[1] = convertBoldToInt(boldBuffer);
                }
                if (strcmp(totalLabelString, buffer) == 0) {
                    fscanf(fp, "%s", totalLabelColor);
                    int colorInt = convertColorToInt(totalLabelColor);
                    if (colorInt != -1) {
                        TOTAL_LABEL_COLOR[0] = colorInt;
                    }
                    fscanf(fp, "%s", boldBuffer);
                    TOTAL_LABEL_COLOR[1] = convertBoldToInt(boldBuffer);
                }
                if (strcmp(cumTransferColumnString, buffer) == 0) {
                    fscanf(fp, "%s", cumTransferColumnColor);
                    int colorInt = convertColorToInt(cumTransferColumnColor);
                    if (colorInt != -1) {
                        CUM_TRANSFER_COLUMN_COLOR[0] = colorInt;
                    }
                    fscanf(fp, "%s", boldBuffer);
                    CUM_TRANSFER_COLUMN_COLOR[1] = convertBoldToInt(boldBuffer);
                }
                if (strcmp(peakTransferColumnString, buffer) == 0) {
                    fscanf(fp, "%s", peakTransferColumnColor);
                    int colorInt = convertColorToInt(peakTransferColumnColor);
                    if (colorInt != -1) {
                        PEAK_TRANSFER_COLUMN_COLOR[0] = colorInt;
                    }
                    fscanf(fp, "%s", boldBuffer);
                    PEAK_TRANSFER_COLUMN_COLOR[1] = convertBoldToInt(boldBuffer);
                }
            }


//            printf("the download bar color is %s\n", downloadBarColor);
//
//            printf("the up bar color is %s\n", uploadBarColor);
//            printf("the host2 color is %s\n", host2Color);
//
//            printf("the peak column is %s\n", peakTransferColumnColor);
//
//            printf("the color int of peak transfer is %d\n", PEAK_TRANSFER_COLUMN_COLOR[0]);
//            printf("the bold is %d\n", PEAK_TRANSFER_COLUMN_COLOR[1]);



//        printf("%s\n", buffer);
//
//        fgets(buffer, 255, (FILE*)fp);
//
//        printf("%s\n", buffer);
//
//        fgets(buffer, 255, (FILE*)fp);
//
//        printf("%s\n", buffer);


        } else {
//        printf(".iftopcolors config file is empty. Resorting to defaults.\n");
        }


    } else {
//       printf(".iftopcolors config file does not exist. Resorting to defaults.\n");
    }


}
