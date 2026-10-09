
#line 1 "src/zxn/zxn_frame.c"

#line 1 "src/zxn/zxn_frame.h"




#line 1 "/home/paul/src/z88dk/lib/config/../..//include/_DEVELOPMENT/common/stdint.h"

 






#line 1 "/home/paul/src/z88dk/lib/config/../..//include/_DEVELOPMENT/common/sys/compiler.h"

 









#line 30 "/home/paul/src/z88dk/lib/config/../..//include/_DEVELOPMENT/common/sys/compiler.h"


 

#line 44 "/home/paul/src/z88dk/lib/config/../..//include/_DEVELOPMENT/common/sys/compiler.h"

 


 



 








 





























#line 9 "/home/paul/src/z88dk/lib/config/../..//include/_DEVELOPMENT/common/stdint.h"

typedef signed char            int8_t;
typedef signed int             int16_t;
typedef signed long            int32_t;

typedef unsigned char          uint8_t;
typedef unsigned int           uint16_t;
typedef unsigned long          uint32_t;

typedef signed char            int_least8_t;
typedef signed int             int_least16_t;
typedef signed long            int_least32_t;

typedef unsigned char          uint_least8_t;
typedef unsigned int           uint_least16_t;
typedef unsigned long          uint_least32_t;

typedef signed int             int_fast8_t;
typedef signed int             int_fast16_t;
typedef signed long            int_fast32_t;

typedef unsigned int           uint_fast8_t;
typedef unsigned int           uint_fast16_t;
typedef unsigned long          uint_fast32_t;
















typedef long long              int64_t;
typedef unsigned long long     uint64_t;

typedef long long              int_least64_t;
typedef unsigned long long     uint_least64_t;

typedef long long              int_fast64_t;
typedef unsigned long long     uint_fast64_t;


















typedef int                    intptr_t;


typedef unsigned int           uintptr_t;










typedef long long              intmax_t;
typedef unsigned long long     uintmax_t;





























































































































 
 






 
 













































#line 5 "src/zxn/zxn_frame.h"

typedef void (*zxn_frame_callback_t)(void);

void zxn_frame_init(void *im2_table, uint8_t vector_byte);
void zxn_frame_start(void);
void zxn_frame_stop(void);
void zxn_frame_wait(void);
uint16_t zxn_frame_count(void);
void zxn_frame_set_callback(zxn_frame_callback_t callback);


#line 2 "src/zxn/zxn_frame.c"


#line 1 "/home/paul/src/z88dk/lib/config/../..//include/_DEVELOPMENT/common/arch/zxn.h"

 






#line 1 "/home/paul/src/z88dk/lib/config/../..//include/_DEVELOPMENT/common/arch.h"

 








 


















































#line 1 "/home/paul/src/z88dk/lib/config/../..//include/_DEVELOPMENT/common/../../../libsrc/newlib/target/zxn/config_zxn.h"









 













































































































































































#line 196 "/home/paul/src/z88dk/lib/config/../..//include/_DEVELOPMENT/common/../../../libsrc/newlib/target/zxn/config_zxn.h"



#line 228 "/home/paul/src/z88dk/lib/config/../..//include/_DEVELOPMENT/common/../../../libsrc/newlib/target/zxn/config_zxn.h"












#line 270 "/home/paul/src/z88dk/lib/config/../..//include/_DEVELOPMENT/common/../../../libsrc/newlib/target/zxn/config_zxn.h"



























































#line 347 "/home/paul/src/z88dk/lib/config/../..//include/_DEVELOPMENT/common/../../../libsrc/newlib/target/zxn/config_zxn.h"

















#line 376 "/home/paul/src/z88dk/lib/config/../..//include/_DEVELOPMENT/common/../../../libsrc/newlib/target/zxn/config_zxn.h"
      



#line 390 "/home/paul/src/z88dk/lib/config/../..//include/_DEVELOPMENT/common/../../../libsrc/newlib/target/zxn/config_zxn.h"
      





  





















   













#line 444 "/home/paul/src/z88dk/lib/config/../..//include/_DEVELOPMENT/common/../../../libsrc/newlib/target/zxn/config_zxn.h"











   


























































































































#line 604 "/home/paul/src/z88dk/lib/config/../..//include/_DEVELOPMENT/common/../../../libsrc/newlib/target/zxn/config_zxn.h"


























#line 663 "/home/paul/src/z88dk/lib/config/../..//include/_DEVELOPMENT/common/../../../libsrc/newlib/target/zxn/config_zxn.h"


















































































































































































































































































































































































































#line 1081 "/home/paul/src/z88dk/lib/config/../..//include/_DEVELOPMENT/common/../../../libsrc/newlib/target/zxn/config_zxn.h"

#line 1098 "/home/paul/src/z88dk/lib/config/../..//include/_DEVELOPMENT/common/../../../libsrc/newlib/target/zxn/config_zxn.h"

#line 1115 "/home/paul/src/z88dk/lib/config/../..//include/_DEVELOPMENT/common/../../../libsrc/newlib/target/zxn/config_zxn.h"

#line 1132 "/home/paul/src/z88dk/lib/config/../..//include/_DEVELOPMENT/common/../../../libsrc/newlib/target/zxn/config_zxn.h"

#line 1148 "/home/paul/src/z88dk/lib/config/../..//include/_DEVELOPMENT/common/../../../libsrc/newlib/target/zxn/config_zxn.h"

#line 1164 "/home/paul/src/z88dk/lib/config/../..//include/_DEVELOPMENT/common/../../../libsrc/newlib/target/zxn/config_zxn.h"

#line 1181 "/home/paul/src/z88dk/lib/config/../..//include/_DEVELOPMENT/common/../../../libsrc/newlib/target/zxn/config_zxn.h"

#line 1198 "/home/paul/src/z88dk/lib/config/../..//include/_DEVELOPMENT/common/../../../libsrc/newlib/target/zxn/config_zxn.h"

#line 1228 "/home/paul/src/z88dk/lib/config/../..//include/_DEVELOPMENT/common/../../../libsrc/newlib/target/zxn/config_zxn.h"

#line 1258 "/home/paul/src/z88dk/lib/config/../..//include/_DEVELOPMENT/common/../../../libsrc/newlib/target/zxn/config_zxn.h"





























































































#line 1371 "/home/paul/src/z88dk/lib/config/../..//include/_DEVELOPMENT/common/../../../libsrc/newlib/target/zxn/config_zxn.h"

#line 1392 "/home/paul/src/z88dk/lib/config/../..//include/_DEVELOPMENT/common/../../../libsrc/newlib/target/zxn/config_zxn.h"

#line 1407 "/home/paul/src/z88dk/lib/config/../..//include/_DEVELOPMENT/common/../../../libsrc/newlib/target/zxn/config_zxn.h"

#line 1422 "/home/paul/src/z88dk/lib/config/../..//include/_DEVELOPMENT/common/../../../libsrc/newlib/target/zxn/config_zxn.h"

#line 1442 "/home/paul/src/z88dk/lib/config/../..//include/_DEVELOPMENT/common/../../../libsrc/newlib/target/zxn/config_zxn.h"

#line 1462 "/home/paul/src/z88dk/lib/config/../..//include/_DEVELOPMENT/common/../../../libsrc/newlib/target/zxn/config_zxn.h"

#line 1611 "/home/paul/src/z88dk/lib/config/../..//include/_DEVELOPMENT/common/../../../libsrc/newlib/target/zxn/config_zxn.h"

#line 1760 "/home/paul/src/z88dk/lib/config/../..//include/_DEVELOPMENT/common/../../../libsrc/newlib/target/zxn/config_zxn.h"








































#line 1815 "/home/paul/src/z88dk/lib/config/../..//include/_DEVELOPMENT/common/../../../libsrc/newlib/target/zxn/config_zxn.h"



















































































#line 1908 "/home/paul/src/z88dk/lib/config/../..//include/_DEVELOPMENT/common/../../../libsrc/newlib/target/zxn/config_zxn.h"






























































#line 1980 "/home/paul/src/z88dk/lib/config/../..//include/_DEVELOPMENT/common/../../../libsrc/newlib/target/zxn/config_zxn.h"


































































#line 2061 "/home/paul/src/z88dk/lib/config/../..//include/_DEVELOPMENT/common/../../../libsrc/newlib/target/zxn/config_zxn.h"

#line 2072 "/home/paul/src/z88dk/lib/config/../..//include/_DEVELOPMENT/common/../../../libsrc/newlib/target/zxn/config_zxn.h"


















#line 2100 "/home/paul/src/z88dk/lib/config/../..//include/_DEVELOPMENT/common/../../../libsrc/newlib/target/zxn/config_zxn.h"



























#line 2138 "/home/paul/src/z88dk/lib/config/../..//include/_DEVELOPMENT/common/../../../libsrc/newlib/target/zxn/config_zxn.h"

#line 2165 "/home/paul/src/z88dk/lib/config/../..//include/_DEVELOPMENT/common/../../../libsrc/newlib/target/zxn/config_zxn.h"

#line 2179 "/home/paul/src/z88dk/lib/config/../..//include/_DEVELOPMENT/common/../../../libsrc/newlib/target/zxn/config_zxn.h"

#line 2190 "/home/paul/src/z88dk/lib/config/../..//include/_DEVELOPMENT/common/../../../libsrc/newlib/target/zxn/config_zxn.h"

#line 2208 "/home/paul/src/z88dk/lib/config/../..//include/_DEVELOPMENT/common/../../../libsrc/newlib/target/zxn/config_zxn.h"

#line 2223 "/home/paul/src/z88dk/lib/config/../..//include/_DEVELOPMENT/common/../../../libsrc/newlib/target/zxn/config_zxn.h"
















































































#line 2313 "/home/paul/src/z88dk/lib/config/../..//include/_DEVELOPMENT/common/../../../libsrc/newlib/target/zxn/config_zxn.h"














#line 2359 "/home/paul/src/z88dk/lib/config/../..//include/_DEVELOPMENT/common/../../../libsrc/newlib/target/zxn/config_zxn.h"

















#line 2386 "/home/paul/src/z88dk/lib/config/../..//include/_DEVELOPMENT/common/../../../libsrc/newlib/target/zxn/config_zxn.h"










#line 2407 "/home/paul/src/z88dk/lib/config/../..//include/_DEVELOPMENT/common/../../../libsrc/newlib/target/zxn/config_zxn.h"







#line 2436 "/home/paul/src/z88dk/lib/config/../..//include/_DEVELOPMENT/common/../../../libsrc/newlib/target/zxn/config_zxn.h"

#line 2449 "/home/paul/src/z88dk/lib/config/../..//include/_DEVELOPMENT/common/../../../libsrc/newlib/target/zxn/config_zxn.h"







#line 2466 "/home/paul/src/z88dk/lib/config/../..//include/_DEVELOPMENT/common/../../../libsrc/newlib/target/zxn/config_zxn.h"






























#line 2506 "/home/paul/src/z88dk/lib/config/../..//include/_DEVELOPMENT/common/../../../libsrc/newlib/target/zxn/config_zxn.h"
















































#line 2569 "/home/paul/src/z88dk/lib/config/../..//include/_DEVELOPMENT/common/../../../libsrc/newlib/target/zxn/config_zxn.h"

































































































#line 2716 "/home/paul/src/z88dk/lib/config/../..//include/_DEVELOPMENT/common/../../../libsrc/newlib/target/zxn/config_zxn.h"



#line 2787 "/home/paul/src/z88dk/lib/config/../..//include/_DEVELOPMENT/common/../../../libsrc/newlib/target/zxn/config_zxn.h"

#line 2817 "/home/paul/src/z88dk/lib/config/../..//include/_DEVELOPMENT/common/../../../libsrc/newlib/target/zxn/config_zxn.h"

























#line 2858 "/home/paul/src/z88dk/lib/config/../..//include/_DEVELOPMENT/common/../../../libsrc/newlib/target/zxn/config_zxn.h"

#line 2875 "/home/paul/src/z88dk/lib/config/../..//include/_DEVELOPMENT/common/../../../libsrc/newlib/target/zxn/config_zxn.h"































#line 2919 "/home/paul/src/z88dk/lib/config/../..//include/_DEVELOPMENT/common/../../../libsrc/newlib/target/zxn/config_zxn.h"

#line 2933 "/home/paul/src/z88dk/lib/config/../..//include/_DEVELOPMENT/common/../../../libsrc/newlib/target/zxn/config_zxn.h"





 
 
 

 


 


 


 


 


 
 
 

 


 
 
 

 


 






 
 
 

 


 





 





 



 





 



 



 



 






 



 







 
 
 

 













#line 63 "/home/paul/src/z88dk/lib/config/../..//include/_DEVELOPMENT/common/arch.h"




#line 9 "/home/paul/src/z88dk/lib/config/../..//include/_DEVELOPMENT/common/arch/zxn.h"


#line 1 "/home/paul/src/z88dk/lib/config/../..//include/_DEVELOPMENT/common/stddef.h"

 











typedef int           ptrdiff_t;



typedef unsigned int  size_t;


typedef unsigned char max_align_t;



typedef unsigned char wchar_t;















#line 11 "/home/paul/src/z88dk/lib/config/../..//include/_DEVELOPMENT/common/arch/zxn.h"

#line 1 "/home/paul/src/z88dk/lib/config/../..//include/_DEVELOPMENT/common/rect.h"

 








 

struct r_Ival8
{
   uint8_t coord;               
   uint8_t width;               
};

struct r_Ival16
{
   uint16_t coord;              
   uint16_t width;              
};

struct r_Rect8
{
   uint8_t x;                   
   uint8_t width;               
   uint8_t y;                   
   uint8_t height;              
};

struct r_Rect16
{
   uint16_t x;                  
   uint16_t width;              
   uint16_t y;                  
   uint16_t height;             
};


#line 12 "/home/paul/src/z88dk/lib/config/../..//include/_DEVELOPMENT/common/arch/zxn.h"

 

 






















 

 

extern unsigned char GLOBAL_ZX_PORT_FE;
extern unsigned char GLOBAL_ZX_PORT_1FFD;
extern unsigned char GLOBAL_ZX_PORT_7FFD;

extern unsigned char GLOBAL_TS_PORT_FE;

extern unsigned char GLOBAL_ZXN_PORT_FE;
extern unsigned char GLOBAL_ZXN_PORT_1FFD;
extern unsigned char GLOBAL_ZXN_PORT_7FFD;
extern unsigned char GLOBAL_ZXN_PORT_DFFD;



 
 




 
 






 




#line 126 "/home/paul/src/z88dk/lib/config/../..//include/_DEVELOPMENT/common/arch/zxn.h"


 

__sfr __at 0xfe IO_FE;

__sfr __at 0xff IO_FF;
__sfr __at  0xff  IO_TIMEX_VIDEO_MODE;

__sfr __banked __at 0x1ffd IO_1FFD;
__sfr __banked __at 0x7ffd IO_7FFD;
__sfr __banked __at 0xdffd IO_DFFD;

 

__sfr __at 0x0f IO_0F;
__sfr __at  0x0f  IO_DAC_L0;

__sfr __at 0x1f IO_1F;
__sfr __at  0x1f  IO_DAC_L1;

__sfr __at 0x4f IO_4F;
__sfr __at  0x4f  IO_DAC_R0;

__sfr __at 0x5f IO_5F;
__sfr __at  0x5f  IO_DAC_R1;

__sfr __at 0xdf IO_DF;
__sfr __at  0xdf  IO_DAC_M0;   

 

__sfr __banked __at 0xfffd IO_FFFD;
__sfr __banked __at  0xfffd  IO_TURBOSOUND;
__sfr __banked __at  0xfffd  IO_AY_REG;

__sfr __banked __at 0xbffd IO_BFFD;
__sfr __banked __at  0xbffd  IO_AY_DAT;

 

__sfr __banked __at 0x123b IO_123B;
__sfr __banked __at  0x123b  IO_LAYER_2_CONFIG;

 

__sfr __banked __at 0x303b IO_303B;
__sfr __banked __at  0x303b  IO_SPRITE_SLOT;
__sfr __banked __at  0x303b  IO_SPRITE_FLAGS;

__sfr __at 0x57 IO_57;
__sfr __at  0x57  IO_SPRITE_ATTRIBUTE;

__sfr __at 0x5b IO_5B;
__sfr __at  0x5b  IO_SPRITE_PATTERN;

 

__sfr __banked __at 0x143b IO_143B;
__sfr __banked __at  0x143b  IO_UART_RX;
__sfr __banked __at  0x143b  IO_UART_BAUD_RATE;

__sfr __banked __at 0x133b IO_133B;
__sfr __banked __at  0x133b  IO_UART_TX;
__sfr __banked __at  0x133b  IO_UART_STATUS;

 

__sfr __at 0x6b IO_6B;
__sfr __at  0x6b  IO_DMA;

 

__sfr __at 0xe3 IO_E3;
__sfr __at  0xe3  IO_DIVIDE_CONTROL;

 

__sfr __at 0xe7 IO_E7;
__sfr __at  0xe7  IO_SPI_CONTROL;
__sfr __at  0xe7  IO_SPI_STATUS;

__sfr __at 0xeb IO_EB;
__sfr __at  0xeb  IO_SPI_DATA;

 

__sfr __banked __at 0x103b IO_103B;
__sfr __banked __at  0x103b  IO_I2C_SCL;   

__sfr __banked __at 0x113b IO_113B;
__sfr __banked __at  0x113b  IO_I2C_SDA;   

 

 
 

__sfr __banked __at  0x103b  IO_LED_L;



 

 





 


#line 252 "/home/paul/src/z88dk/lib/config/../..//include/_DEVELOPMENT/common/arch/zxn.h"

#line 266 "/home/paul/src/z88dk/lib/config/../..//include/_DEVELOPMENT/common/arch/zxn.h"
 






 



 











 















 

















 







 

 










 


#line 363 "/home/paul/src/z88dk/lib/config/../..//include/_DEVELOPMENT/common/arch/zxn.h"

#line 380 "/home/paul/src/z88dk/lib/config/../..//include/_DEVELOPMENT/common/arch/zxn.h"
 









 















#line 421 "/home/paul/src/z88dk/lib/config/../..//include/_DEVELOPMENT/common/arch/zxn.h"



















































































#line 514 "/home/paul/src/z88dk/lib/config/../..//include/_DEVELOPMENT/common/arch/zxn.h"
 




 















 

 







__sfr __banked __at 0x243b IO_243B;
__sfr __banked __at  0x243b  IO_NEXTREG_REG;

__sfr __banked __at 0x253b IO_253B;
__sfr __banked __at  0x253b  IO_NEXTREG_DAT;


 


#line 565 "/home/paul/src/z88dk/lib/config/../..//include/_DEVELOPMENT/common/arch/zxn.h"










#line 586 "/home/paul/src/z88dk/lib/config/../..//include/_DEVELOPMENT/common/arch/zxn.h"







#line 615 "/home/paul/src/z88dk/lib/config/../..//include/_DEVELOPMENT/common/arch/zxn.h"

#line 628 "/home/paul/src/z88dk/lib/config/../..//include/_DEVELOPMENT/common/arch/zxn.h"






#line 644 "/home/paul/src/z88dk/lib/config/../..//include/_DEVELOPMENT/common/arch/zxn.h"



















 





 






#line 686 "/home/paul/src/z88dk/lib/config/../..//include/_DEVELOPMENT/common/arch/zxn.h"
















































#line 749 "/home/paul/src/z88dk/lib/config/../..//include/_DEVELOPMENT/common/arch/zxn.h"



























 

 






















extern unsigned char ZXN_READ_REG(unsigned char reg)  __preserves_regs(a,d,e,h,iyl,iyh);
extern unsigned char ZXN_READ_REG_fastcall(unsigned char reg) __preserves_regs(a,d,e,h,iyl,iyh) __z88dk_fastcall;




extern void    ZXN_WRITE_REG(unsigned char reg,unsigned char val)    __preserves_regs(a,d,e,iyl,iyh);
extern void    ZXN_WRITE_REG_callee(unsigned char reg,unsigned char val)    __preserves_regs(a,d,e,iyl,iyh) __z88dk_callee;




 

extern unsigned char    ZXN_READ_MMU0(void)    __preserves_regs(d,e,h,iyl,iyh);

extern unsigned char    ZXN_READ_MMU1(void)    __preserves_regs(d,e,h,iyl,iyh);

extern unsigned char    ZXN_READ_MMU2(void)    __preserves_regs(d,e,h,iyl,iyh);

extern unsigned char    ZXN_READ_MMU3(void)    __preserves_regs(d,e,h,iyl,iyh);

extern unsigned char    ZXN_READ_MMU4(void)    __preserves_regs(d,e,h,iyl,iyh);

extern unsigned char    ZXN_READ_MMU5(void)    __preserves_regs(d,e,h,iyl,iyh);

extern unsigned char    ZXN_READ_MMU6(void)    __preserves_regs(d,e,h,iyl,iyh);

extern unsigned char    ZXN_READ_MMU7(void)    __preserves_regs(d,e,h,iyl,iyh);



#line 843 "/home/paul/src/z88dk/lib/config/../..//include/_DEVELOPMENT/common/arch/zxn.h"

#line 854 "/home/paul/src/z88dk/lib/config/../..//include/_DEVELOPMENT/common/arch/zxn.h"

 




extern void *zxn_addr_from_mmu(unsigned char mmu)  __preserves_regs(b,c,d,e,iyl,iyh);
extern void *zxn_addr_from_mmu_fastcall(unsigned char mmu) __preserves_regs(b,c,d,e,iyl,iyh) __z88dk_fastcall;







extern unsigned char zxn_mmu_from_addr(void *addr)  __preserves_regs(b,c,d,e,iyl,iyh);
extern unsigned char zxn_mmu_from_addr_fastcall(void *addr) __preserves_regs(b,c,d,e,iyl,iyh) __z88dk_fastcall;




extern void    *zxn_addr_in_mmu(unsigned char mmu,void *addr)    __preserves_regs(b,c,d,iyl,iyh);
extern void    *zxn_addr_in_mmu_callee(unsigned char mmu,void *addr)    __preserves_regs(b,c,d,iyl,iyh) __z88dk_callee;







extern unsigned long zxn_addr_from_page(unsigned char page)  __preserves_regs(b,c,iyl,iyh);
extern unsigned long zxn_addr_from_page_fastcall(unsigned char page) __preserves_regs(b,c,iyl,iyh) __z88dk_fastcall;







extern unsigned char zxn_page_from_addr(unsigned long addr)  __preserves_regs(b,c,iyl,iyh);
extern unsigned char zxn_page_from_addr_fastcall(unsigned long addr) __preserves_regs(b,c,d,e,iyl,iyh) __z88dk_fastcall;








extern void zxn_read_mmu_state(void *dst)  __preserves_regs(iyl,iyh);
extern void zxn_read_mmu_state_fastcall(void *dst) __preserves_regs(a,iyl,iyh) __z88dk_fastcall;







extern void zxn_write_mmu_state(void *src)  __preserves_regs(iyl,iyh);
extern void zxn_write_mmu_state_fastcall(void *src) __preserves_regs(iyl,iyh) __z88dk_fastcall;








extern void zxn_write_bank_state(unsigned int state)  __preserves_regs(d,e,iyl,iyh);
extern void zxn_write_bank_state_fastcall(unsigned int state) __preserves_regs(d,e,h,l,iyl,iyh) __z88dk_fastcall;




extern unsigned int    zxn_read_sysvar_bank_state(void)    __preserves_regs(b,c,d,e,iyl,iyh);




extern void zxn_write_sysvar_bank_state(unsigned int state)  __preserves_regs(b,c,d,e,iyl,iyh);
extern void zxn_write_sysvar_bank_state_fastcall(unsigned int state) __preserves_regs(b,c,d,e,h,l,iyl,iyh) __z88dk_fastcall;







extern unsigned int zxn_mangle_bank_state(unsigned int state)  __preserves_regs(b,c,d,e,iyl,iyh);
extern unsigned int zxn_mangle_bank_state_fastcall(unsigned int state) __preserves_regs(b,c,d,e,iyl,iyh) __z88dk_fastcall;





 

 
 
 
 
 




extern unsigned char check_version_core(unsigned int core_version)  __preserves_regs(d,iyl,iyh);
extern unsigned char check_version_core_fastcall(unsigned int core_version) __preserves_regs(d,iyl,iyh) __z88dk_fastcall;







extern unsigned char check_version_esxdos(unsigned int esxdos_version)  __preserves_regs(iyl,iyh);
extern unsigned char check_version_esxdos_fastcall(unsigned int esxdos_version) __preserves_regs(iyl,iyh) __z88dk_fastcall;







extern unsigned char check_version_nextzxos(unsigned int nextzxos_version)  __preserves_regs(iyl,iyh);
extern unsigned char check_version_nextzxos_fastcall(unsigned int nextzxos_version) __preserves_regs(iyl,iyh) __z88dk_fastcall;





 

 

struct zxtapehdr
{
	unsigned char hdtype;       
	unsigned char hdname[10];   
	unsigned int  hdlen;        
	unsigned int  hdadd;        
	unsigned int  hdvars;       
};




extern unsigned int    zx_tape_load_block(void *dst,unsigned int len,unsigned char type)    __preserves_regs(iyl,iyh);
extern unsigned int    zx_tape_load_block_callee(void *dst,unsigned int len,unsigned char type)    __preserves_regs(iyl,iyh) __z88dk_callee;



extern unsigned int    zx_tape_save_block(void *src,unsigned int len,unsigned char type)    __preserves_regs(iyl,iyh);
extern unsigned int    zx_tape_save_block_callee(void *src,unsigned int len,unsigned char type)    __preserves_regs(iyl,iyh) __z88dk_callee;



extern unsigned int    zx_tape_verify_block(void *dst,unsigned int len,unsigned char type)    __preserves_regs(iyl,iyh);
extern unsigned int    zx_tape_verify_block_callee(void *dst,unsigned int len,unsigned char type)    __preserves_regs(iyl,iyh) __z88dk_callee;




 

 










extern void ts_vmod(unsigned char mode)  __preserves_regs(b,c,d,e,iyl,iyh);
extern void ts_vmod_fastcall(unsigned char mode) __preserves_regs(b,c,d,e,iyl,iyh) __z88dk_fastcall;





 




extern void zx_border(unsigned char colour)  __preserves_regs(b,c,d,e,iyl,iyh);
extern void zx_border_fastcall(unsigned char colour) __preserves_regs(b,c,d,e,h,iyl,iyh) __z88dk_fastcall;







extern void zx_cls(unsigned char attr)  __preserves_regs(iyl,iyh);
extern void zx_cls_fastcall(unsigned char attr) __preserves_regs(iyl,iyh) __z88dk_fastcall;







extern void zx_cls_attr(unsigned char attr)  __preserves_regs(iyl,iyh);
extern void zx_cls_attr_fastcall(unsigned char attr) __preserves_regs(iyl,iyh) __z88dk_fastcall;







extern void zx_cls_pix(unsigned char pix)  __preserves_regs(iyl,iyh);
extern void zx_cls_pix_fastcall(unsigned char pix) __preserves_regs(iyl,iyh) __z88dk_fastcall;




extern void    zx_cls_wc(struct r_Rect8 *r,unsigned char attr)   ;
extern void    zx_cls_wc_callee(struct r_Rect8 *r,unsigned char attr)    __z88dk_callee;



extern void    zx_cls_wc_attr(struct r_Rect8 *r,unsigned char attr)   ;
extern void    zx_cls_wc_attr_callee(struct r_Rect8 *r,unsigned char attr)    __z88dk_callee;



extern void    zx_cls_wc_pix(struct r_Rect8 *r,unsigned char pix)   ;
extern void    zx_cls_wc_pix_callee(struct r_Rect8 *r,unsigned char pix)    __z88dk_callee;



extern void    zx_scroll_up(unsigned char rows,unsigned char attr)    __preserves_regs(iyl,iyh);
extern void    zx_scroll_up_callee(unsigned char rows,unsigned char attr)    __preserves_regs(iyl,iyh) __z88dk_callee;



extern void    zx_scroll_up_attr(unsigned char rows,unsigned char attr)    __preserves_regs(iyl,iyh);
extern void    zx_scroll_up_attr_callee(unsigned char rows,unsigned char attr)    __preserves_regs(iyl,iyh) __z88dk_callee;



extern void    zx_scroll_up_pix(unsigned char rows,unsigned char pix)    __preserves_regs(iyl,iyh);
extern void    zx_scroll_up_pix_callee(unsigned char rows,unsigned char pix)    __preserves_regs(iyl,iyh) __z88dk_callee;



extern void    zx_scroll_wc_up(struct r_Rect8 *r,unsigned char rows,unsigned char attr)   ;
extern void    zx_scroll_wc_up_callee(struct r_Rect8 *r,unsigned char rows,unsigned char attr)    __z88dk_callee;



extern void    zx_scroll_wc_up_attr(struct r_Rect8 *r,unsigned char rows,unsigned char attr)   ;
extern void    zx_scroll_wc_up_attr_callee(struct r_Rect8 *r,unsigned char rows,unsigned char attr)    __z88dk_callee;



extern void    zx_scroll_wc_up_pix(struct r_Rect8 *r,unsigned char rows,unsigned char pix)   ;
extern void    zx_scroll_wc_up_pix_callee(struct r_Rect8 *r,unsigned char rows,unsigned char pix)    __z88dk_callee;





















extern void    zx_visit_wc_attr(struct r_Rect8 *r,void (*visit)(unsigned char *))   ;
extern void    zx_visit_wc_attr_callee(struct r_Rect8 *r,void (*visit)(unsigned char *))    __z88dk_callee;



extern void    zx_visit_wc_pix(struct r_Rect8 *r,void (*visit)(unsigned char *))   ;
extern void    zx_visit_wc_pix_callee(struct r_Rect8 *r,void (*visit)(unsigned char *))    __z88dk_callee;





















 




extern void tshc_cls(unsigned char attr)  __preserves_regs(iyl,iyh);
extern void tshc_cls_fastcall(unsigned char attr) __preserves_regs(iyl,iyh) __z88dk_fastcall;







extern void tshc_cls_attr(unsigned char attr)  __preserves_regs(iyl,iyh);
extern void tshc_cls_attr_fastcall(unsigned char attr) __preserves_regs(iyl,iyh) __z88dk_fastcall;







extern void tshc_cls_pix(unsigned char pix)  __preserves_regs(iyl,iyh);
extern void tshc_cls_pix_fastcall(unsigned char pix) __preserves_regs(iyl,iyh) __z88dk_fastcall;




extern void    tshc_cls_wc(struct r_Rect8 *r,unsigned char attr)   ;
extern void    tshc_cls_wc_callee(struct r_Rect8 *r,unsigned char attr)    __z88dk_callee;



extern void    tshc_cls_wc_attr(struct r_Rect8 *r,unsigned char attr)   ;
extern void    tshc_cls_wc_attr_callee(struct r_Rect8 *r,unsigned char attr)    __z88dk_callee;



extern void    tshc_cls_wc_pix(struct r_Rect8 *r,unsigned char pix)   ;
extern void    tshc_cls_wc_pix_callee(struct r_Rect8 *r,unsigned char pix)    __z88dk_callee;



extern void    tshc_scroll_up(unsigned char rows,unsigned char attr)    __preserves_regs(iyl,iyh);
extern void    tshc_scroll_up_callee(unsigned char rows,unsigned char attr)    __preserves_regs(iyl,iyh) __z88dk_callee;



extern void    tshc_scroll_up_attr(unsigned char rows,unsigned char attr)    __preserves_regs(iyl,iyh);
extern void    tshc_scroll_up_attr_callee(unsigned char rows,unsigned char attr)    __preserves_regs(iyl,iyh) __z88dk_callee;



extern void    tshc_scroll_up_pix(unsigned char rows,unsigned char pix)    __preserves_regs(iyl,iyh);
extern void    tshc_scroll_up_pix_callee(unsigned char rows,unsigned char pix)    __preserves_regs(iyl,iyh) __z88dk_callee;



extern void    tshc_scroll_wc_up(struct r_Rect8 *r,unsigned char rows,unsigned char attr)   ;
extern void    tshc_scroll_wc_up_callee(struct r_Rect8 *r,unsigned char rows,unsigned char attr)    __z88dk_callee;



extern void    tshc_scroll_wc_up_attr(struct r_Rect8 *r,unsigned char rows,unsigned char attr)   ;
extern void    tshc_scroll_wc_up_attr_callee(struct r_Rect8 *r,unsigned char rows,unsigned char attr)    __z88dk_callee;



extern void    tshc_scroll_wc_up_pix(struct r_Rect8 *r,unsigned char rows,unsigned char pix)   ;
extern void    tshc_scroll_wc_up_pix_callee(struct r_Rect8 *r,unsigned char rows,unsigned char pix)    __z88dk_callee;





















extern void    tshc_visit_wc_attr(struct r_Rect8 *r,void (*visit)(unsigned char *))   ;
extern void    tshc_visit_wc_attr_callee(struct r_Rect8 *r,void (*visit)(unsigned char *))    __z88dk_callee;



extern void    tshc_visit_wc_pix(struct r_Rect8 *r,void (*visit)(unsigned char *))   ;
extern void    tshc_visit_wc_pix_callee(struct r_Rect8 *r,void (*visit)(unsigned char *))    __z88dk_callee;





















 




extern void tshr_cls(unsigned char attr)  __preserves_regs(iyl,iyh);
extern void tshr_cls_fastcall(unsigned char attr) __preserves_regs(iyl,iyh) __z88dk_fastcall;







extern void tshr_cls_attr(unsigned char attr)  __preserves_regs(iyl,iyh);
extern void tshr_cls_attr_fastcall(unsigned char attr) __preserves_regs(iyl,iyh) __z88dk_fastcall;







extern void tshr_cls_pix(unsigned char pix)  __preserves_regs(iyl,iyh);
extern void tshr_cls_pix_fastcall(unsigned char pix) __preserves_regs(iyl,iyh) __z88dk_fastcall;




extern void    tshr_cls_wc_pix(struct r_Rect8 *r,unsigned char pix)   ;
extern void    tshr_cls_wc_pix_callee(struct r_Rect8 *r,unsigned char pix)    __z88dk_callee;



extern void    tshr_scroll_up(unsigned char rows,unsigned char attr)    __preserves_regs(iyl,iyh);
extern void    tshr_scroll_up_callee(unsigned char rows,unsigned char attr)    __preserves_regs(iyl,iyh) __z88dk_callee;



extern void    tshr_scroll_up_pix(unsigned char rows,unsigned char pix)    __preserves_regs(iyl,iyh);
extern void    tshr_scroll_up_pix_callee(unsigned char rows,unsigned char pix)    __preserves_regs(iyl,iyh) __z88dk_callee;



extern void    tshr_scroll_wc_up_pix(struct r_Rect8 *r,unsigned char rows,unsigned char pix)   ;
extern void    tshr_scroll_wc_up_pix_callee(struct r_Rect8 *r,unsigned char rows,unsigned char pix)    __z88dk_callee;
















extern void    tshr_visit_wc_pix(struct r_Rect8 *r,void (*visit)(unsigned char *))   ;
extern void    tshr_visit_wc_pix_callee(struct r_Rect8 *r,void (*visit)(unsigned char *))    __z88dk_callee;
















 

 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 




extern unsigned char zx_aaddr2cx(void *aaddr)  __preserves_regs(b,c,d,e,iyl,iyh);
extern unsigned char zx_aaddr2cx_fastcall(void *aaddr) __preserves_regs(b,c,d,e,h,iyl,iyh) __z88dk_fastcall;







extern unsigned char zx_aaddr2cy(void *aaddr)  __preserves_regs(b,c,d,e,iyl,iyh);
extern unsigned char zx_aaddr2cy_fastcall(void *aaddr) __preserves_regs(b,c,d,e,iyl,iyh) __z88dk_fastcall;







extern unsigned char zx_aaddr2px(void *aaddr)  __preserves_regs(b,c,d,e,iyl,iyh);
extern unsigned char zx_aaddr2px_fastcall(void *aaddr) __preserves_regs(b,c,d,e,h,iyl,iyh) __z88dk_fastcall;







extern unsigned char zx_aaddr2py(void *aaddr)  __preserves_regs(b,c,d,e,iyl,iyh);
extern unsigned char zx_aaddr2py_fastcall(void *aaddr) __preserves_regs(b,c,d,e,iyl,iyh) __z88dk_fastcall;







extern unsigned char *zx_aaddr2saddr(void *aaddr)  __preserves_regs(b,c,d,e,iyl,iyh);
extern unsigned char *zx_aaddr2saddr_fastcall(void *aaddr) __preserves_regs(b,c,d,e,iyl,iyh) __z88dk_fastcall;







extern unsigned char *zx_aaddrcdown(void *aaddr)  __preserves_regs(b,c,d,e,iyl,iyh);
extern unsigned char *zx_aaddrcdown_fastcall(void *aaddr) __preserves_regs(b,c,d,e,iyl,iyh) __z88dk_fastcall;







extern unsigned char *zx_aaddrcleft(void *aaddr)  __preserves_regs(b,c,d,e,iyl,iyh);
extern unsigned char *zx_aaddrcleft_fastcall(void *aaddr) __preserves_regs(b,c,d,e,iyl,iyh) __z88dk_fastcall;







extern unsigned char *zx_aaddrcright(void *aaddr)  __preserves_regs(b,c,d,e,iyl,iyh);
extern unsigned char *zx_aaddrcright_fastcall(void *aaddr) __preserves_regs(b,c,d,e,iyl,iyh) __z88dk_fastcall;







extern unsigned char *zx_aaddrcup(void *aaddr)  __preserves_regs(b,c,d,e,iyl,iyh);
extern unsigned char *zx_aaddrcup_fastcall(void *aaddr) __preserves_regs(b,c,d,e,iyl,iyh) __z88dk_fastcall;







extern unsigned char zx_bitmask2px(unsigned char bitmask)  __preserves_regs(b,c,d,e,iyl,iyh);
extern unsigned char zx_bitmask2px_fastcall(unsigned char bitmask) __preserves_regs(b,c,d,e,h,iyl,iyh) __z88dk_fastcall;




extern unsigned char    *zx_cxy2aaddr(unsigned char x,unsigned char y)    __preserves_regs(b,c,d,e,iyl,iyh);
extern unsigned char    *zx_cxy2aaddr_callee(unsigned char x,unsigned char y)    __preserves_regs(b,c,d,e,iyl,iyh) __z88dk_callee;



extern unsigned char    *zx_cxy2saddr(unsigned char x,unsigned char y)    __preserves_regs(b,c,d,e,iyl,iyh);
extern unsigned char    *zx_cxy2saddr_callee(unsigned char x,unsigned char y)    __preserves_regs(b,c,d,e,iyl,iyh) __z88dk_callee;






extern unsigned char *zx_cy2aaddr(unsigned char y)  __preserves_regs(b,c,d,e,iyl,iyh);
extern unsigned char *zx_cy2aaddr_fastcall(unsigned char y) __preserves_regs(b,c,d,e,iyl,iyh) __z88dk_fastcall;







extern unsigned char *zx_cy2saddr(unsigned char y)  __preserves_regs(b,c,d,e,iyl,iyh);
extern unsigned char *zx_cy2saddr_fastcall(unsigned char y) __preserves_regs(b,c,d,e,iyl,iyh) __z88dk_fastcall;







extern unsigned char zx_px2bitmask(unsigned char x)  __preserves_regs(b,c,d,e,iyl,iyh);
extern unsigned char zx_px2bitmask_fastcall(unsigned char x) __preserves_regs(b,c,d,e,iyl,iyh) __z88dk_fastcall;




extern unsigned char    *zx_pxy2aaddr(unsigned char x,unsigned char y)    __preserves_regs(b,c,d,e,iyl,iyh);
extern unsigned char    *zx_pxy2aaddr_callee(unsigned char x,unsigned char y)    __preserves_regs(b,c,d,e,iyl,iyh) __z88dk_callee;



extern unsigned char    *zx_pxy2saddr(unsigned char x,unsigned char y)    __preserves_regs(b,c,d,e,iyl,iyh);
extern unsigned char    *zx_pxy2saddr_callee(unsigned char x,unsigned char y)    __preserves_regs(b,c,d,e,iyl,iyh) __z88dk_callee;






extern unsigned char *zx_py2aaddr(unsigned char y)  __preserves_regs(b,c,d,e,iyl,iyh);
extern unsigned char *zx_py2aaddr_fastcall(unsigned char y) __preserves_regs(b,c,d,e,iyl,iyh) __z88dk_fastcall;







extern unsigned char *zx_py2saddr(unsigned char y)  __preserves_regs(b,c,d,e,iyl,iyh);
extern unsigned char *zx_py2saddr_fastcall(unsigned char y) __preserves_regs(b,c,d,e,iyl,iyh) __z88dk_fastcall;







extern unsigned char *zx_saddr2aaddr(void *saddr)  __preserves_regs(b,c,d,e,iyl,iyh);
extern unsigned char *zx_saddr2aaddr_fastcall(void *saddr) __preserves_regs(b,c,d,e,l,iyl,iyh) __z88dk_fastcall;







extern unsigned char zx_saddr2cx(void *saddr)  __preserves_regs(b,c,d,e,iyl,iyh);
extern unsigned char zx_saddr2cx_fastcall(void *saddr) __preserves_regs(b,c,d,e,h,iyl,iyh) __z88dk_fastcall;







extern unsigned char zx_saddr2cy(void *saddr)  __preserves_regs(b,c,d,e,iyl,iyh);
extern unsigned char zx_saddr2cy_fastcall(void *saddr) __preserves_regs(b,c,d,e,h,iyl,iyh) __z88dk_fastcall;







extern unsigned char zx_saddr2px(void *saddr)  __preserves_regs(b,c,d,e,iyl,iyh);
extern unsigned char zx_saddr2px_fastcall(void *saddr) __preserves_regs(b,c,d,e,h,iyl,iyh) __z88dk_fastcall;







extern unsigned char zx_saddr2py(void *saddr)  __preserves_regs(b,c,d,e,iyl,iyh);
extern unsigned char zx_saddr2py_fastcall(void *saddr) __preserves_regs(b,c,d,e,h,iyl,iyh) __z88dk_fastcall;







extern unsigned char *zx_saddrcdown(void *saddr)  __preserves_regs(b,c,d,e,iyl,iyh);
extern unsigned char *zx_saddrcdown_fastcall(void *saddr) __preserves_regs(b,c,d,e,iyl,iyh) __z88dk_fastcall;







extern unsigned char *zx_saddrcleft(void *saddr)  __preserves_regs(b,c,d,e,iyl,iyh);
extern unsigned char *zx_saddrcleft_fastcall(void *saddr) __preserves_regs(b,c,d,e,iyl,iyh) __z88dk_fastcall;







extern unsigned char *zx_saddrcright(void *saddr)  __preserves_regs(b,c,d,e,iyl,iyh);
extern unsigned char *zx_saddrcright_fastcall(void *saddr) __preserves_regs(b,c,d,e,iyl,iyh) __z88dk_fastcall;







extern unsigned char *zx_saddrcup(void *saddr)  __preserves_regs(b,c,d,e,iyl,iyh);
extern unsigned char *zx_saddrcup_fastcall(void *saddr) __preserves_regs(b,c,d,e,iyl,iyh) __z88dk_fastcall;







extern unsigned char *zx_saddrpdown(void *saddr)  __preserves_regs(b,c,d,e,iyl,iyh);
extern unsigned char *zx_saddrpdown_fastcall(void *saddr) __preserves_regs(b,c,d,e,iyl,iyh) __z88dk_fastcall;




extern unsigned char    *zx_saddrpleft(void *saddr,unsigned char bitmask)    __preserves_regs(b,c,iyl,iyh);
extern unsigned char    *zx_saddrpleft_callee(void *saddr,unsigned char bitmask)    __preserves_regs(b,c,iyl,iyh) __z88dk_callee;



extern unsigned char    *zx_saddrpright(void *saddr,unsigned char bitmask)    __preserves_regs(b,c,iyl,iyh);
extern unsigned char    *zx_saddrpright_callee(void *saddr,unsigned char bitmask)    __preserves_regs(b,c,iyl,iyh) __z88dk_callee;






extern unsigned char *zx_saddrpup(void *saddr)  __preserves_regs(b,c,d,e,iyl,iyh);
extern unsigned char *zx_saddrpup_fastcall(void *saddr) __preserves_regs(b,c,d,e,iyl,iyh) __z88dk_fastcall;





 




extern unsigned char tshc_aaddr2cx(void *aaddr)  __preserves_regs(b,c,d,e,iyl,iyh);
extern unsigned char tshc_aaddr2cx_fastcall(void *aaddr) __preserves_regs(b,c,d,e,h,iyl,iyh) __z88dk_fastcall;







extern unsigned char tshc_aaddr2cy(void *aaddr)  __preserves_regs(b,c,d,e,iyl,iyh);
extern unsigned char tshc_aaddr2cy_fastcall(void *aaddr) __preserves_regs(b,c,d,e,h,iyl,iyh) __z88dk_fastcall;







extern unsigned char tshc_aaddr2px(void *aaddr)  __preserves_regs(b,c,d,e,iyl,iyh);
extern unsigned char tshc_aaddr2px_fastcall(void *aaddr) __preserves_regs(b,c,d,e,h,iyl,iyh) __z88dk_fastcall;







extern unsigned char tshc_aaddr2py(void *aaddr)  __preserves_regs(b,c,d,e,iyl,iyh);
extern unsigned char tshc_aaddr2py_fastcall(void *aaddr) __preserves_regs(b,c,d,e,h,iyl,iyh) __z88dk_fastcall;







extern unsigned char *tshc_aaddr2saddr(void *aaddr)  __preserves_regs(b,c,d,e,iyl,iyh);
extern unsigned char *tshc_aaddr2saddr_fastcall(void *aaddr) __preserves_regs(a,b,c,d,e,l,iyl,iyh) __z88dk_fastcall;







extern unsigned char *tshc_aaddrcdown(void *aaddr)  __preserves_regs(b,c,d,e,iyl,iyh);
extern unsigned char *tshc_aaddrcdown_fastcall(void *aaddr) __preserves_regs(b,c,d,e,iyl,iyh) __z88dk_fastcall;







extern unsigned char *tshc_aaddrcleft(void *aaddr)  __preserves_regs(b,c,d,e,iyl,iyh);
extern unsigned char *tshc_aaddrcleft_fastcall(void *aaddr) __preserves_regs(b,c,d,e,iyl,iyh) __z88dk_fastcall;







extern unsigned char *tshc_aaddrcright(void *aaddr)  __preserves_regs(b,c,d,e,iyl,iyh);
extern unsigned char *tshc_aaddrcright_fastcall(void *aaddr) __preserves_regs(b,c,d,e,iyl,iyh) __z88dk_fastcall;







extern unsigned char *tshc_aaddrcup(void *aaddr)  __preserves_regs(b,c,d,e,iyl,iyh);
extern unsigned char *tshc_aaddrcup_fastcall(void *aaddr) __preserves_regs(b,c,d,e,iyl,iyh) __z88dk_fastcall;







extern unsigned char *tshc_aaddrpdown(void *aaddr)  __preserves_regs(b,c,d,e,iyl,iyh);
extern unsigned char *tshc_aaddrpdown_fastcall(void *aaddr) __preserves_regs(b,c,d,e,iyl,iyh) __z88dk_fastcall;




extern unsigned char    *tshc_aaddrpleft(void *aaddr,unsigned char bitmask)    __preserves_regs(b,c,iyl,iyh);
extern unsigned char    *tshc_aaddrpleft_callee(void *aaddr,unsigned char bitmask)    __preserves_regs(b,c,iyl,iyh) __z88dk_callee;



extern unsigned char    *tshc_aaddrpright(void *aaddr,unsigned char bitmask)    __preserves_regs(b,c,iyl,iyh);
extern unsigned char    *tshc_aaddrpright_callee(void *aaddr,unsigned char bitmask)    __preserves_regs(b,c,iyl,iyh) __z88dk_callee;






extern unsigned char *tshc_aaddrpup(void *aaddr)  __preserves_regs(b,c,d,e,iyl,iyh);
extern unsigned char *tshc_aaddrpup_fastcall(void *aaddr) __preserves_regs(b,c,d,e,iyl,iyh) __z88dk_fastcall;







extern unsigned char tshc_bitmask2px(unsigned char bitmask)  __preserves_regs(b,c,d,e,iyl,iyh);
extern unsigned char tshc_bitmask2px_fastcall(unsigned char bitmask) __preserves_regs(b,c,d,e,h,iyl,iyh) __z88dk_fastcall;




extern unsigned char    *tshc_cxy2aaddr(unsigned char x,unsigned char y)    __preserves_regs(b,c,d,e,iyl,iyh);
extern unsigned char    *tshc_cxy2aaddr_callee(unsigned char x,unsigned char y)    __preserves_regs(b,c,d,e,iyl,iyh) __z88dk_callee;



extern unsigned char    *tshc_cxy2saddr(unsigned char x,unsigned char y)    __preserves_regs(b,c,d,e,iyl,iyh);
extern unsigned char    *tshc_cxy2saddr_callee(unsigned char x,unsigned char y)    __preserves_regs(b,c,d,e,iyl,iyh) __z88dk_callee;






extern unsigned char *tshc_cy2aaddr(unsigned char y)  __preserves_regs(b,c,d,e,iyl,iyh);
extern unsigned char *tshc_cy2aaddr_fastcall(unsigned char y) __preserves_regs(b,c,d,e,iyl,iyh) __z88dk_fastcall;







extern unsigned char *tshc_cy2saddr(unsigned char y)  __preserves_regs(b,c,d,e,iyl,iyh);
extern unsigned char *tshc_cy2saddr_fastcall(unsigned char y) __preserves_regs(b,c,d,e,iyl,iyh) __z88dk_fastcall;







extern unsigned char tshc_px2bitmask(unsigned char x)  __preserves_regs(b,c,d,e,iyl,iyh);
extern unsigned char tshc_px2bitmask_fastcall(unsigned char x) __preserves_regs(b,c,d,e,iyl,iyh) __z88dk_fastcall;




extern unsigned char    *tshc_pxy2aaddr(unsigned char x,unsigned char y)    __preserves_regs(b,c,d,e,iyl,iyh);
extern unsigned char    *tshc_pxy2aaddr_callee(unsigned char x,unsigned char y)    __preserves_regs(b,c,d,e,iyl,iyh) __z88dk_callee;



extern unsigned char    *tshc_pxy2saddr(unsigned char x,unsigned char y)    __preserves_regs(b,c,d,e,iyl,iyh);
extern unsigned char    *tshc_pxy2saddr_callee(unsigned char x,unsigned char y)    __preserves_regs(b,c,d,e,iyl,iyh) __z88dk_callee;






extern unsigned char *tshc_py2aaddr(unsigned char y)  __preserves_regs(b,c,d,e,iyl,iyh);
extern unsigned char *tshc_py2aaddr_fastcall(unsigned char y) __preserves_regs(b,c,d,e,iyl,iyh) __z88dk_fastcall;







extern unsigned char *tshc_py2saddr(unsigned char y)  __preserves_regs(b,c,d,e,iyl,iyh);
extern unsigned char *tshc_py2saddr_fastcall(unsigned char y) __preserves_regs(b,c,d,e,iyl,iyh) __z88dk_fastcall;







extern unsigned char *tshc_saddr2aaddr(void *saddr)  __preserves_regs(b,c,d,e,iyl,iyh);
extern unsigned char *tshc_saddr2aaddr_fastcall(void *saddr) __preserves_regs(a,b,c,d,e,l,iyl,iyh) __z88dk_fastcall;







extern unsigned char tshc_saddr2cx(void *saddr)  __preserves_regs(b,c,d,e,iyl,iyh);
extern unsigned char tshc_saddr2cx_fastcall(void *saddr) __preserves_regs(b,c,d,e,h,iyl,iyh) __z88dk_fastcall;







extern unsigned char tshc_saddr2cy(void *saddr)  __preserves_regs(b,c,d,e,iyl,iyh);
extern unsigned char tshc_saddr2cy_fastcall(void *saddr) __preserves_regs(b,c,d,e,h,iyl,iyh) __z88dk_fastcall;







extern unsigned char tshc_saddr2px(void *saddr)  __preserves_regs(b,c,d,e,iyl,iyh);
extern unsigned char tshc_saddr2px_fastcall(void *saddr) __preserves_regs(b,c,d,e,h,iyl,iyh) __z88dk_fastcall;







extern unsigned char tshc_saddr2py(void *saddr)  __preserves_regs(b,c,d,e,iyl,iyh);
extern unsigned char tshc_saddr2py_fastcall(void *saddr) __preserves_regs(b,c,d,e,h,iyl,iyh) __z88dk_fastcall;







extern unsigned char *tshc_saddrcdown(void *saddr)  __preserves_regs(b,c,d,e,iyl,iyh);
extern unsigned char *tshc_saddrcdown_fastcall(void *saddr) __preserves_regs(b,c,d,e,iyl,iyh) __z88dk_fastcall;







extern unsigned char *tshc_saddrcleft(void *saddr)  __preserves_regs(b,c,d,e,iyl,iyh);
extern unsigned char *tshc_saddrcleft_fastcall(void *saddr) __preserves_regs(b,c,d,e,iyl,iyh) __z88dk_fastcall;







extern unsigned char *tshc_saddrcright(void *saddr)  __preserves_regs(b,c,d,e,iyl,iyh);
extern unsigned char *tshc_saddrcright_fastcall(void *saddr) __preserves_regs(b,c,d,e,iyl,iyh) __z88dk_fastcall;







extern unsigned char *tshc_saddrcup(void *saddr)  __preserves_regs(b,c,d,e,iyl,iyh);
extern unsigned char *tshc_saddrcup_fastcall(void *saddr) __preserves_regs(b,c,d,e,iyl,iyh) __z88dk_fastcall;







extern unsigned char *tshc_saddrpdown(void *saddr)  __preserves_regs(b,c,d,e,iyl,iyh);
extern unsigned char *tshc_saddrpdown_fastcall(void *saddr) __preserves_regs(b,c,d,e,iyl,iyh) __z88dk_fastcall;




extern unsigned char    *tshc_saddrpleft(void *saddr,unsigned char bitmask)    __preserves_regs(b,c,iyl,iyh);
extern unsigned char    *tshc_saddrpleft_callee(void *saddr,unsigned char bitmask)    __preserves_regs(b,c,iyl,iyh) __z88dk_callee;



extern unsigned char    *tshc_saddrpright(void *saddr,unsigned char bitmask)    __preserves_regs(b,c,iyl,iyh);
extern unsigned char    *tshc_saddrpright_callee(void *saddr,unsigned char bitmask)    __preserves_regs(b,c,iyl,iyh) __z88dk_callee;






extern unsigned char *tshc_saddrpup(void *saddr)  __preserves_regs(b,c,d,e,iyl,iyh);
extern unsigned char *tshc_saddrpup_fastcall(void *saddr) __preserves_regs(b,c,d,e,iyl,iyh) __z88dk_fastcall;





 




extern unsigned char tshr_bitmask2px(unsigned char bitmask)  __preserves_regs(b,c,d,e,iyl,iyh);
extern unsigned char tshr_bitmask2px_fastcall(unsigned char bitmask) __preserves_regs(b,c,d,e,h,iyl,iyh) __z88dk_fastcall;




extern unsigned char    *tshr_cxy2saddr(unsigned char x,unsigned char y)    __preserves_regs(b,c,d,e,iyl,iyh);
extern unsigned char    *tshr_cxy2saddr_callee(unsigned char x,unsigned char y)    __preserves_regs(b,c,d,e,iyl,iyh) __z88dk_callee;






extern unsigned char *tshr_cy2saddr(unsigned char y)  __preserves_regs(b,c,d,e,iyl,iyh);
extern unsigned char *tshr_cy2saddr_fastcall(unsigned char y) __preserves_regs(b,c,d,e,iyl,iyh) __z88dk_fastcall;







extern unsigned char tshr_px2bitmask(unsigned char x)  __preserves_regs(b,c,d,e,iyl,iyh);
extern unsigned char tshr_px2bitmask_fastcall(unsigned char x) __preserves_regs(b,c,d,e,iyl,iyh) __z88dk_fastcall;




extern unsigned char    *tshr_pxy2saddr(unsigned int x,unsigned char y)    __preserves_regs(iyl,iyh);
extern unsigned char    *tshr_pxy2saddr_callee(unsigned int x,unsigned char y)    __preserves_regs(iyl,iyh) __z88dk_callee;






extern unsigned char *tshr_py2saddr(unsigned char y)  __preserves_regs(b,c,d,e,iyl,iyh);
extern unsigned char *tshr_py2saddr_fastcall(unsigned char y) __preserves_regs(b,c,d,e,iyl,iyh) __z88dk_fastcall;







extern unsigned char tshr_saddr2cx(void *saddr)  __preserves_regs(b,c,d,e,iyl,iyh);
extern unsigned char tshr_saddr2cx_fastcall(void *saddr) __preserves_regs(b,c,d,e,h,iyl,iyh) __z88dk_fastcall;







extern unsigned char tshr_saddr2cy(void *saddr)  __preserves_regs(b,c,d,e,iyl,iyh);
extern unsigned char tshr_saddr2cy_fastcall(void *saddr) __preserves_regs(b,c,d,e,h,iyl,iyh) __z88dk_fastcall;







extern unsigned int tshr_saddr2px(void *saddr)  __preserves_regs(b,c,d,e,iyl,iyh);
extern unsigned int tshr_saddr2px_fastcall(void *saddr) __preserves_regs(b,c,d,e,iyl,iyh) __z88dk_fastcall;







extern unsigned char tshr_saddr2py(void *saddr)  __preserves_regs(b,c,d,e,iyl,iyh);
extern unsigned char tshr_saddr2py_fastcall(void *saddr) __preserves_regs(b,c,d,e,h,iyl,iyh) __z88dk_fastcall;







extern unsigned char *tshr_saddrcdown(void *saddr)  __preserves_regs(b,c,d,e,iyl,iyh);
extern unsigned char *tshr_saddrcdown_fastcall(void *saddr) __preserves_regs(b,c,d,e,iyl,iyh) __z88dk_fastcall;







extern unsigned char *tshr_saddrcleft(void *saddr)  __preserves_regs(b,c,d,e,iyl,iyh);
extern unsigned char *tshr_saddrcleft_fastcall(void *saddr) __preserves_regs(b,c,d,e,iyl,iyh) __z88dk_fastcall;







extern unsigned char *tshr_saddrcright(void *saddr)  __preserves_regs(b,c,d,e,iyl,iyh);
extern unsigned char *tshr_saddrcright_fastcall(void *saddr) __preserves_regs(b,c,d,e,iyl,iyh) __z88dk_fastcall;







extern unsigned char *tshr_saddrcup(void *saddr)  __preserves_regs(b,c,d,e,iyl,iyh);
extern unsigned char *tshr_saddrcup_fastcall(void *saddr) __preserves_regs(b,c,d,e,iyl,iyh) __z88dk_fastcall;







extern unsigned char *tshr_saddrpdown(void *saddr)  __preserves_regs(b,c,d,e,iyl,iyh);
extern unsigned char *tshr_saddrpdown_fastcall(void *saddr) __preserves_regs(b,c,d,e,iyl,iyh) __z88dk_fastcall;




extern unsigned char    *tshr_saddrpleft(void *saddr,unsigned char bitmask)    __preserves_regs(b,c,iyl,iyh);
extern unsigned char    *tshr_saddrpleft_callee(void *saddr,unsigned char bitmask)    __preserves_regs(b,c,iyl,iyh) __z88dk_callee;



extern unsigned char    *tshr_saddrpright(void *saddr,unsigned char bitmask)    __preserves_regs(b,c,iyl,iyh);
extern unsigned char    *tshr_saddrpright_callee(void *saddr,unsigned char bitmask)    __preserves_regs(b,c,iyl,iyh) __z88dk_callee;






extern unsigned char *tshr_saddrpup(void *saddr)  __preserves_regs(b,c,d,e,iyl,iyh);
extern unsigned char *tshr_saddrpup_fastcall(void *saddr) __preserves_regs(b,c,d,e,iyl,iyh) __z88dk_fastcall;





 

extern int    zx_pattern_fill(unsigned char x,unsigned char y,void *pattern,unsigned int depth)   ;
extern int    zx_pattern_fill_callee(unsigned char x,unsigned char y,void *pattern,unsigned int depth)    __z88dk_callee;





#line 4 "src/zxn/zxn_frame.c"

#line 1 "/home/paul/src/z88dk/lib/config/../..//include/_DEVELOPMENT/common/intrinsic.h"

 









 

#pragma disable_warning 84
#pragma disable_warning 112


















extern void intrinsic_ldi(void*,void*) __z88dk_callee;



extern void intrinsic_outi(void*) __z88dk_fastcall;



extern void intrinsic_ini(void*) __z88dk_fastcall;





extern void    intrinsic_stub(void)    __preserves_regs(a,b,c,d,e,h,l);


extern void    intrinsic_di(void)    __preserves_regs(a,b,c,d,e,h,l);

extern void    intrinsic_ei(void)    __preserves_regs(a,b,c,d,e,h,l);

extern void    intrinsic_halt(void)    __preserves_regs(a,b,c,d,e,h,l);

extern void    intrinsic_reti(void)    __preserves_regs(a,b,c,d,e,h,l);

extern void    intrinsic_retn(void)    __preserves_regs(a,b,c,d,e,h,l);

extern void    intrinsic_im_0(void)    __preserves_regs(a,b,c,d,e,h,l);

extern void    intrinsic_im_1(void)    __preserves_regs(a,b,c,d,e,h,l);

extern void    intrinsic_im_2(void)    __preserves_regs(a,b,c,d,e,h,l);

extern void    intrinsic_nop(void)    __preserves_regs(a,b,c,d,e,h,l);


extern void    intrinsic_ex_de_hl(void)    __preserves_regs(a,b,c);

extern void    intrinsic_exx(void)    __preserves_regs(a);


extern void    *intrinsic_return_bc(void)    __preserves_regs(a,b,c,d,e);

extern void    *intrinsic_return_de(void)    __preserves_regs(a,b,c,d,e);





extern unsigned int intrinsic_swap_endian_16(unsigned int n)  __preserves_regs(b,c,d,e);
extern unsigned int intrinsic_swap_endian_16_fastcall(unsigned int n) __preserves_regs(b,c,d,e) __z88dk_fastcall;







extern unsigned long intrinsic_swap_endian_32(unsigned long n)  __preserves_regs(b,c);
extern unsigned long intrinsic_swap_endian_32_fastcall(unsigned long n) __preserves_regs(b,c) __z88dk_fastcall;







extern unsigned long intrinsic_swap_word_32(unsigned long n)  __preserves_regs(a,b,c);
extern unsigned long intrinsic_swap_word_32_fastcall(unsigned long n) __preserves_regs(a,b,c) __z88dk_fastcall;



























#line 5 "src/zxn/zxn_frame.c"

static volatile uint16_t frame_count;
static zxn_frame_callback_t frame_callback;

static void zxn_frame_isr(void)
{
    M_PRESERVE_MAIN;

    ++frame_count;

    if (frame_callback != 0)
    {
        frame_callback();
    }

    M_RESTORE_MAIN;
}

void zxn_frame_init(void *im2_table, uint8_t vector_byte)
{
    intrinsic_di();
    frame_count = 0;
    frame_callback = 0;
    zx_im2_init(im2_table, vector_byte);
    add_raster_int(zxn_frame_isr);
}

void zxn_frame_start(void)
{
    intrinsic_ei();
}

void zxn_frame_stop(void)
{
    intrinsic_di();
}

void zxn_frame_wait(void)
{
    intrinsic_halt();
}

uint16_t zxn_frame_count(void)
{
    return frame_count;
}

void zxn_frame_set_callback(zxn_frame_callback_t callback)
{
    frame_callback = callback;
}
