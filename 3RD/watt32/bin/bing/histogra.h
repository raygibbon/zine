
/* histogram.h -- include file for histogram facility
 *
 */

#include <tcp.h>  /* 'u_char' etc. */

#define HCB struct hcb_stru
HCB {      /*  HCB structure -- Histogram Control Block */
     char *hcb_link ;
     char  hcb_name[12];
     int   hcb_scale ;
     int   hcb_orig  ;
     u_char hcb_flag  ; 
#define HCFclear  128
#define HCFactive  64
#define HCFcreate  32
#define HCFsearch  16
#define HCFhalfw   08
     u_char hcb_spare;
     short  hcb_mbin  ;    /* bins are numbered 0, 1, ... mbin */
     int    hcb_bins[1] ;  /* mbin+1 integer bins */
     } ; 

#ifndef MIN
#define MIN(a,b) (((a) < (b)) ? (a) : (b))
#endif

#ifndef MAX
#define MAX(a,b) (((a) > (b)) ? (a) : (b))
#endif

