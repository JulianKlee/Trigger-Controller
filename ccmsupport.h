#ifndef _WW_CCMSUPPORT_H_
#define _WW_CCMSUPPORT_H_

#include <ch.h>

# define _ccm_data_  __attribute__(( __section__(".ccmdata") ))
# define _ccm_bss_   __attribute__(( __section__(".ccmbss") ))

// NOTE: Known symbols in ChibiOS and their size: 
// 
//  9144    <rb>: MAC: not CCM compatible
//    96    <rd>: MAC: not CCM compatible
//  6096    <tb>: MAC: not CCM compatible
//    64    <td>: MAC: not CCM compatible
//   516    <u>:  SDC (unaligned buffer): not CCM compatible
//   784    <USBD1>: probably not CCM compat.
//   600    <SDU1>: probably not CCM compat.
//   564    <SDC_FS>: probably not CCM compat.
//  4116    <ram_heap>: ?????
//   192    <Files>: FatFS (?)

#endif  /* _WW_CCMSUPPORT_H_ */
