
#include <stdio.h>
#include <sys/types.h>
#include <sys/param.h>

#include "histogra.h"

#define HISTCHAR 'X'


/*
 *           histogram( HCB-ptr, value )
 *
 *   Increment bin in histogram area (HCB) pointed to by HCB-ptr.
 *   Bin number is given by: 
 *          b = min(MAXBIN, max(0, 
 *                  int(value/SCALE - ORIGIN) )) 
 *
 *   Hence, there are MAXBIN+1 bins, and extreme values count in
 *   the first (0) and last (MAXBIN) bins.
 */
histogram( hcbp, value ) 
register HCB *hcbp ;
int  value ;
    {
    register int x ;
    
    if (hcbp->hcb_flag & HCFclear) {
        /* clear bins */
        bzero( hcbp->hcb_bins, ( (hcbp->hcb_mbin+1)<<2) );
        hcbp->hcb_flag ^= HCFclear ;
    }
    
    x =  (hcbp->hcb_scale == 0) ? -99999999 : 
                       value/hcbp->hcb_scale - hcbp->hcb_orig ;
    
    hcbp->hcb_bins[ (x > hcbp->hcb_mbin) ? hcbp->hcb_mbin :
                                 (x < 0) ? 0 : x ]++ ;
}


    /* 
     *   ASCII Histogram facility.
     *
     */

HCB *
init_histogram(scale, origin, maxbin, name)
    int scale, origin, maxbin;
    char *name;
    {
    int size = sizeof(HCB) + maxbin*sizeof(int);
    HCB *hcbp = (HCB *) malloc(size);

    if (hcbp == NULL) return(NULL);
    bzero(hcbp, size);
    hcbp->hcb_scale = scale;
    hcbp->hcb_orig = origin;
    hcbp->hcb_mbin = maxbin;
    strncpy(hcbp->hcb_name, name, MIN(strlen(name), sizeof(hcbp->hcb_name)));
    return(hcbp);
}

/*  
 *  histdisplay( HCB-ptr, title-ptr, width, depth, FILE-ptr)
 *
 *
 */
histdisplay( hcbp, title, width, depth, fp)
register HCB *hcbp ;
char *title ;
register FILE *fp ;
int width, depth ;

    {
    register int *dp, i, j;
    int  n, s, maxbin, minbin, maxcount, ag, basis ;
    
         /* supply some defaults for width and depth */
    if (width <10) width = 50 ;
    if (depth < 4) depth = 32 ;
    
    fprintf( fp, title, hcbp->hcb_name) ;
    dp =  (int *) hcbp->hcb_bins ;
    
        /* Find lowest and highest non-empty bins */
    for (i = hcbp->hcb_mbin; i >= 0 && dp[i] == 0; i--) ;
    if (i < 0) {
        fprintf(fp, "NO DATA\n") ;
        return ;
    }
    maxbin = i ;
    for (i = 0; dp[i] == 0; i++) ;
    minbin = i;
    
        /* If less than half of space from 0 to first non-empty bit
         *   will be blank, display  starting at 0.
         */
    if (maxbin > 2*minbin + hcbp->hcb_orig)  
            minbin = 0 ;

        /* Aggregate bins, if necessary, to fit into specified depth. */
    if ((ag = (1 + (maxbin-minbin)/(depth-2) )) > 1) {
        int l = 0;
        
        j = 0 ;
        while (l <= maxbin)  {
            int s = 0 ;
            for (i = 0; i < ag; i++) s+= dp[l+i] ;
            dp[j++] = s ;
            l += ag ;
        }  
        maxbin = j-1 ;
        minbin /= ag ;
    }
    
        /* Find max count in any bin */
    j = 0 ;
    for (i = 0; i <= maxbin; i++) 
        if (dp[i] > j) j = dp[i] ;
    maxcount = j ; 
        
        /* Scale to fit max bin into 'width', with nice scale */  
    s = nicescale( maxcount, width) ;   
    
        /* Adjust limits to include one zero bin on each end */
    if (maxbin < hcbp->hcb_mbin)  dp[++maxbin] = 0 ;
    if (minbin > 0) minbin-- ;
    
        /* Now print histogram... */
    basis = hcbp->hcb_orig * hcbp->hcb_scale + (ag*hcbp->hcb_scale)/2  ;
    for (i = minbin; i <= maxbin; i++) {
        fprintf(fp, "%5d|%3d ", ag*i* hcbp->hcb_scale + basis,
            dp[i]) ;
        for (j = dp[i]/s ; j > 0 ; j--) 
            putc(HISTCHAR, fp) ;
        putc('\n', fp) ;
    }
    fprintf(fp, "Each %c = %d counts\n", HISTCHAR,  s) ; 
                
    fprintf(fp, "\n") ;
}

    
int nicescale( maxval, targsize)
register int maxval, targsize ;
    {
    register int i, j ;
    
    i = 1 ;
    j = 5*targsize ;
    while (i*j < maxval) i *= 10 ;  /* Power-of-10 scale factor */
    j = maxval / i ;
    return( (j > 2*targsize) ?  5*i : (j > targsize) ? 2*i  : i );
}
