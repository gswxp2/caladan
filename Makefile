ROOT_PATH=.
include $(ROOT_PATH)/build/shared.mk

DPDK_PATH = dpdk
CHECKFLAGS = -D__CHECKER__ -Waddress-space

ifneq ($(TCP_RX_STATS),)
CFLAGS += -DTCP_RX_STATS
endif
CFLAGS += -DVDEV_SERVER
# libbase.a - the base library
base_src = $(wildcard base/*.c)
base_obj = $(base_src:.c=.o)

#libnet.a - a packet/networking utility library
net_src = $(wildcard net/*.c)
net_obj = $(net_src:.c=.o)

# iokernel - a soft-NIC service
iokernel_src = $(wildcard iokernel/*.c)
iokernel_obj = $(iokernel_src:.c=.o)
$(iokernel_obj): INC += -I$(DPDK_PATH)/build/include

# runtime - a user-level threading and networking library
runtime_src = $(wildcard runtime/*.c) $(wildcard runtime/net/*.c)
runtime_src += $(wildcard runtime/net/directpath/*.c)
runtime_src += $(wildcard runtime/net/directpath/mlx5/*.c)
runtime_src += $(wildcard runtime/rpc/*.c)
runtime_asm = $(wildcard runtime/*.S)
runtime_obj = $(runtime_src:.c=.o) $(runtime_asm:.S=.o)

# test cases
test_src = $(wildcard tests/*.c)
test_obj = $(test_src:.c=.o)
test_targets = $(basename $(test_src))

# pcm lib
PCM_DEPS = $(ROOT_PATH)/deps/pcm/libPCM.a
PCM_LIBS = -lm -lstdc++

# dpdk libs
DPDK_LIBS =-I~/caladan-all-vdev/caladan/dpdk/build/include -include ~/caladan-all-vdev/caladan/dpdk/build/include/rte_config.h -D_GNU_SOURCE -O3 -W -Wall -Wstrict-prototypes -Wmissing-prototypes -Wmissing-declarations -Wold-style-definition -Wpointer-arith -Wcast-align -Wnested-externs -Wcast-qual -Wformat-nonliteral -Wformat-security -Wundef -Wwrite-strings -Wdeprecated -Werror -Wimplicit-fallthrough=2 -Wno-format-truncation -Wno-address-of-packed-member -L~/caladan-all-vdev/caladan/dpdk/build/lib 
DPDK_LIBS +=-Wl,-lpthread 
DPDK_LIBS +=-Wl,-lrte_flow_classify 
DPDK_LIBS +=-Wl,--whole-archive 
DPDK_LIBS +=-Wl,-lrte_pipeline 
DPDK_LIBS +=-Wl,--no-whole-archive 
DPDK_LIBS +=-Wl,--whole-archive 
DPDK_LIBS +=-Wl,-lrte_table 
DPDK_LIBS +=-Wl,--no-whole-archive 
DPDK_LIBS +=-Wl,--whole-archive 
DPDK_LIBS +=-Wl,-lrte_port 
DPDK_LIBS +=-Wl,--no-whole-archive 
DPDK_LIBS +=-Wl,-lrte_pdump 
DPDK_LIBS +=-Wl,-lrte_distributor 
DPDK_LIBS +=-Wl,-lrte_ip_frag 
DPDK_LIBS +=-Wl,-lrte_meter 
DPDK_LIBS +=-Wl,-lrte_fib 
DPDK_LIBS +=-Wl,-lrte_rib 
DPDK_LIBS +=-Wl,-lrte_lpm 
DPDK_LIBS +=-Wl,-lrte_acl 
DPDK_LIBS +=-Wl,-lrte_jobstats 
DPDK_LIBS +=-Wl,-lrte_metrics 
DPDK_LIBS +=-Wl,-lrte_bitratestats 
DPDK_LIBS +=-Wl,-lrte_latencystats 
DPDK_LIBS +=-Wl,-lrte_power 
DPDK_LIBS +=-Wl,-lrte_efd 
DPDK_LIBS +=-Wl,-lrte_bpf 
DPDK_LIBS +=-Wl,-lrte_ipsec 
DPDK_LIBS +=-Wl,--whole-archive 
DPDK_LIBS +=-Wl,-lrte_cfgfile 
DPDK_LIBS +=-Wl,-lrte_gro 
DPDK_LIBS +=-Wl,-lrte_gso 
DPDK_LIBS +=-Wl,-lrte_hash 
DPDK_LIBS +=-Wl,-lrte_member 
DPDK_LIBS +=-Wl,-lrte_vhost 
DPDK_LIBS +=-Wl,-lrte_kvargs 
DPDK_LIBS +=-Wl,-lrte_mbuf 
DPDK_LIBS +=-Wl,-lrte_net 
DPDK_LIBS +=-Wl,-lrte_ethdev 
DPDK_LIBS +=-Wl,-lrte_bbdev 
DPDK_LIBS +=-Wl,-lrte_cryptodev 
DPDK_LIBS +=-Wl,-lrte_security 
DPDK_LIBS +=-Wl,-lrte_compressdev 
DPDK_LIBS +=-Wl,-lrte_eventdev 
DPDK_LIBS +=-Wl,-lrte_rawdev 
DPDK_LIBS +=-Wl,-lrte_timer 
DPDK_LIBS +=-Wl,-lrte_mempool 
DPDK_LIBS +=-Wl,-lrte_stack 
DPDK_LIBS +=-Wl,-lrte_mempool_ring 
DPDK_LIBS +=-Wl,-lrte_mempool_octeontx2 
DPDK_LIBS +=-Wl,-lrte_ring 
DPDK_LIBS +=-Wl,-lrte_pci 
DPDK_LIBS +=-Wl,-lrte_eal 
DPDK_LIBS +=-Wl,-lrte_cmdline 
DPDK_LIBS +=-Wl,-lrte_reorder 
DPDK_LIBS +=-Wl,-lrte_sched 
DPDK_LIBS +=-Wl,-lrte_rcu 
DPDK_LIBS +=-Wl,-lrte_kni 
DPDK_LIBS +=-Wl,-lrte_common_cpt 
DPDK_LIBS +=-Wl,-lrte_common_octeontx 
DPDK_LIBS +=-Wl,-lrte_common_octeontx2 
DPDK_LIBS +=-Wl,-lrte_common_dpaax 
DPDK_LIBS +=-Wl,-lrte_bus_pci 
DPDK_LIBS +=-Wl,-lrte_bus_vdev 
DPDK_LIBS +=-Wl,-lrte_bus_dpaa 
DPDK_LIBS +=-Wl,-lrte_bus_fslmc 
DPDK_LIBS +=-Wl,-lrte_mempool_bucket 
DPDK_LIBS +=-Wl,-lrte_mempool_stack 
DPDK_LIBS +=-Wl,-lrte_mempool_dpaa 
DPDK_LIBS +=-Wl,-lrte_mempool_dpaa2 
DPDK_LIBS +=-Wl,-lrte_pmd_af_packet 
DPDK_LIBS +=-Wl,-lrte_pmd_ark 
DPDK_LIBS +=-Wl,-lrte_pmd_atlantic 
DPDK_LIBS +=-Wl,-lrte_pmd_avp 
DPDK_LIBS +=-Wl,-lrte_pmd_axgbe 
DPDK_LIBS +=-Wl,-lrte_pmd_bnxt 
DPDK_LIBS +=-Wl,-lrte_pmd_bond 
DPDK_LIBS +=-Wl,-lrte_pmd_cxgbe 
DPDK_LIBS +=-Wl,-lrte_pmd_dpaa 
DPDK_LIBS +=-Wl,-lrte_pmd_dpaa2 
DPDK_LIBS +=-Wl,-lrte_pmd_e1000 
DPDK_LIBS +=-Wl,-lrte_pmd_ena 
DPDK_LIBS +=-Wl,-lrte_pmd_enetc 
DPDK_LIBS +=-Wl,-lrte_pmd_enic 
DPDK_LIBS +=-Wl,-lrte_pmd_fm10k 
DPDK_LIBS +=-Wl,-lrte_pmd_failsafe 
DPDK_LIBS +=-Wl,-lrte_pmd_hinic 
DPDK_LIBS +=-Wl,-lrte_pmd_hns3 
DPDK_LIBS +=-Wl,-lrte_pmd_iavf 
DPDK_LIBS +=-Wl,-lrte_pmd_ice 
DPDK_LIBS +=-Wl,-lrte_pmd_ixgbe 
DPDK_LIBS +=-Wl,-lrte_pmd_kni 
DPDK_LIBS +=-Wl,-lrte_pmd_lio 
DPDK_LIBS +=-Wl,-lrte_pmd_memif 
DPDK_LIBS +=-Wl,-lrte_pmd_mlx5 
DPDK_LIBS +=-Wl,-libverbs 
DPDK_LIBS +=-Wl,-lmlx5 
DPDK_LIBS +=-Wl,-lrte_pmd_nfp 
DPDK_LIBS +=-Wl,-lrte_pmd_null 
DPDK_LIBS +=-Wl,-lrte_pmd_octeontx2 
DPDK_LIBS +=-Wl,-lrte_pmd_qede 
DPDK_LIBS +=-Wl,-lrte_pmd_ring 
DPDK_LIBS +=-Wl,-lrte_pmd_softnic 
DPDK_LIBS +=-Wl,-lrte_pmd_sfc_efx 
DPDK_LIBS +=-Wl,-lrte_pmd_tap 
DPDK_LIBS +=-Wl,-lrte_pmd_thunderx_nicvf 
DPDK_LIBS +=-Wl,-lrte_pmd_vdev_netvsc 
DPDK_LIBS +=-Wl,-lrte_pmd_virtio 
DPDK_LIBS +=-Wl,-lrte_pmd_vhost 
DPDK_LIBS +=-Wl,-lrte_pmd_ifc 
DPDK_LIBS +=-Wl,-lrte_pmd_vmxnet3_uio 
DPDK_LIBS +=-Wl,-lrte_bus_vmbus 
DPDK_LIBS +=-Wl,-lrte_pmd_netvsc 
DPDK_LIBS +=-Wl,-lrte_pmd_bbdev_null 
DPDK_LIBS +=-Wl,-lrte_pmd_bbdev_fpga_lte_fec 
DPDK_LIBS +=-Wl,-lrte_pmd_bbdev_turbo_sw 
DPDK_LIBS +=-Wl,-lrte_pmd_null_crypto 
DPDK_LIBS +=-Wl,-lrte_pmd_nitrox 
DPDK_LIBS +=-Wl,-lrte_pmd_octeontx_crypto 
DPDK_LIBS +=-Wl,-lrte_pmd_octeontx2_crypto 
DPDK_LIBS +=-Wl,-lrte_pmd_crypto_scheduler 
DPDK_LIBS +=-Wl,-lrte_pmd_dpaa2_sec 
DPDK_LIBS +=-Wl,-lrte_pmd_dpaa_sec 
DPDK_LIBS +=-Wl,-lrte_pmd_caam_jr 
DPDK_LIBS +=-Wl,-lrte_pmd_virtio_crypto 
DPDK_LIBS +=-Wl,-lrte_pmd_octeontx_zip 
DPDK_LIBS +=-Wl,-lrte_pmd_qat 
DPDK_LIBS +=-Wl,-lrte_pmd_skeleton_event 
DPDK_LIBS +=-Wl,-lrte_pmd_sw_event 
DPDK_LIBS +=-Wl,-lrte_pmd_dsw_event 
DPDK_LIBS +=-Wl,-lrte_pmd_octeontx_ssovf 
DPDK_LIBS +=-Wl,-lrte_pmd_dpaa_event 
DPDK_LIBS +=-Wl,-lrte_pmd_dpaa2_event 
DPDK_LIBS +=-Wl,-lrte_mempool_octeontx 
DPDK_LIBS +=-Wl,-lrte_pmd_octeontx 
DPDK_LIBS +=-Wl,-lrte_pmd_octeontx2_event 
DPDK_LIBS +=-Wl,-lrte_pmd_opdl_event 
DPDK_LIBS +=-Wl,-lrte_rawdev_skeleton 
DPDK_LIBS +=-Wl,-lrte_rawdev_dpaa2_cmdif 
DPDK_LIBS +=-Wl,-lrte_rawdev_dpaa2_qdma 
DPDK_LIBS +=-Wl,-lrte_bus_ifpga 
DPDK_LIBS +=-Wl,-lrte_rawdev_ntb 
DPDK_LIBS +=-Wl,-lrte_rawdev_octeontx2_dma 
DPDK_LIBS +=-Wl,--no-whole-archive 
DPDK_LIBS +=-Wl,-lrt 
DPDK_LIBS +=-Wl,-lm 
DPDK_LIBS +=-Wl,-lnuma 
DPDK_LIBS +=-Wl,-ldl 
DPDK_LIBS +=-Wl,-export-dynamic 
DPDK_LIBS +=-Wl,-export-dynamic -L~/caladan-all-vdev/caladan/dpdk/examples/helloworld/build/lib -L~caladan-all-vdev/caladan/dpdk/build/lib 
DPDK_LIBS +=-Wl,--as-needed  

# DPDK_LIBS= -L$(DPDK_PATH)/build/lib
# DPDK_LIBS += -Wl,-whole-archive -lrte_pmd_e1000 -Wl,-no-whole-archive
# DPDK_LIBS += -Wl,-whole-archive -lrte_pmd_ixgbe -Wl,-no-whole-archive
# DPDK_LIBS += -Wl,-whole-archive -lrte_mempool_ring -Wl,-no-whole-archive
# DPDK_LIBS += -Wl,-whole-archive -lrte_pmd_tap -Wl,-no-whole-archive
# DPDK_LIBS += -ldpdk
# DPDK_LIBS += -lrte_eal
# DPDK_LIBS += -lrte_ethdev
# DPDK_LIBS += -lrte_hash
# DPDK_LIBS += -lrte_mbuf
# DPDK_LIBS += -lrte_mempool
# DPDK_LIBS += -lrte_mempool_stack
# DPDK_LIBS += -lrte_ring
# # additional libs for running with Mellanox NICs
# ifeq ($(CONFIG_MLX5),y)
# DPDK_LIBS += $(MLX5_LIBS) -lrte_pmd_mlx5
# $(iokernel_obj): INC += $(MLX5_INC)
# else
# ifeq ($(CONFIG_MLX4),y)
# DPDK_LIBS += -lrte_pmd_mlx4 -libverbs -lmlx4
# endif
# endif

# must be first
all: libbase.a libnet.a libruntime.a iokerneld $(test_targets)

libbase.a: $(base_obj)
	$(AR) rcs $@ $^

libnet.a: $(net_obj)
	$(AR) rcs $@ $^

libruntime.a: $(runtime_obj)
	$(AR) rcs $@ $^

iokerneld: $(iokernel_obj) libbase.a libnet.a base/base.ld $(PCM_DEPS)
	$(LD) $(LDFLAGS) -o $@ $(iokernel_obj) libbase.a libnet.a $(DPDK_LIBS) \
	$(PCM_DEPS) $(PCM_LIBS) -lpthread -lnuma -ldl

$(test_targets): $(test_obj) libbase.a libruntime.a libnet.a base/base.ld
	$(LD) $(LDFLAGS) -o $@ $@.o $(RUNTIME_LIBS)

# general build rules for all targets
src = $(base_src) $(net_src) $(runtime_src) $(iokernel_src) $(test_src)
asm = $(runtime_asm)
obj = $(src:.c=.o) $(asm:.S=.o)
dep = $(obj:.o=.d)

ifneq ($(MAKECMDGOALS),clean)
-include $(dep)   # include all dep files in the makefile
endif

# rule to generate a dep file by using the C preprocessor
# (see man cpp for details on the -MM and -MT options)
%.d: %.c
	@$(CC) $(CFLAGS) $< -MM -MT $(@:.d=.o) >$@
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@
%.d: %.S
	@$(CC) $(CFLAGS) $< -MM -MT $(@:.d=.o) >$@
%.o: %.S
	$(CC) $(CFLAGS) -c $< -o $@

# prints sparse checker tool output
sparse: $(src)
	$(foreach f,$^,$(SPARSE) $(filter-out -std=gnu11, $(CFLAGS)) $(CHECKFLAGS) $(f);)

.PHONY: submodules
submodules:
	$(ROOT_PATH)/build/init_submodules.sh

.PHONY: clean
clean:
	rm -f $(obj) $(dep) libbase.a libnet.a libruntime.a \
	iokerneld $(test_targets)
