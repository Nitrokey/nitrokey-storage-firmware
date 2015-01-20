


#include <stdio.h>
#include "string.h"
#include "global.h"
#include "CONVERT_USB_LOG.h"


/******************************************************************************

  Convert USB log

	Convertiert das Log des USB Loggers für die Simulation

******************************************************************************/

int ConvertUsbLog (char *Filename_u8)
{
    FILE *fd_in;
    FILE *fd_out;
    char szOutFilename[200];
    char szData[1001];
    char *p;
    char *p1;
    int n;


    fd_in = fopen (Filename_u8, "r");
    if (NULL == fd_in)
    {
        printf ("Can't open %s\n", Filename_u8);
        return (FALSE);
    }


    _snprintf (szOutFilename, 199, "%s.txt", Filename_u8);
    fd_out = fopen (szOutFilename, "w");
    if (NULL == fd_out)
    {
        printf ("Can't open %s for write\n", szOutFilename);
        fclose (fd_in);
        return (FALSE);
    }


    while (0 == feof (fd_in))
    {
        fgets (szData, 1000, fd_in);
        p = strstr (szData, "OUT txn");
        if (NULL != p)
        {
            p = &p[8];
            n = strlen (p);
            if (0 != n)
            {
                p[n - 1] = 0;
            }
            fprintf (fd_out, "  \"O - %s\",\n", p);
        }
        p = strstr (szData, "IN txn");
        if (NULL != strstr (szData, "IN txn"))
        {
            p = &p[7];
            p1 = strstr (szData, " POLL],");
            if (NULL != p1)
            {
                p = &p1[7];
            }
            n = strlen (p);
            if (0 != n)
            {
                p[n - 1] = 0;
            }
            fprintf (fd_out, "  \"I - %s\",\n", p);
        }
    }
    fclose (fd_out);
    fclose (fd_in);

    return (TRUE);
}
