#
_XDCBUILDCOUNT = 
ifneq (,$(findstring path,$(_USEXDCENV_)))
override XDCPATH = /home/marco/ti/simplelink_msp432p4_sdk_3_40_01_02/source;/home/marco/ti/simplelink_msp432p4_sdk_3_40_01_02/kernel/tirtos/packages
override XDCROOT = /home/marco/ti/ccs/ccs1240/xdctools_3_62_01_16_core
override XDCBUILDCFG = ./config.bld
endif
ifneq (,$(findstring args,$(_USEXDCENV_)))
override XDCARGS = 
override XDCTARGETS = 
endif
#
ifeq (0,1)
PKGPATH = /home/marco/ti/simplelink_msp432p4_sdk_3_40_01_02/source;/home/marco/ti/simplelink_msp432p4_sdk_3_40_01_02/kernel/tirtos/packages;/home/marco/ti/ccs/ccs1240/xdctools_3_62_01_16_core/packages;..
HOSTOS = Linux
endif
