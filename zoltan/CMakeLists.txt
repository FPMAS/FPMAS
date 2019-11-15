CONFIGURE_FILE(include/Zoltan_config.h.in Zoltan_config.h)

SET(CMAKE_EXPORT_COMPILE_COMMANDS YES)

INCLUDE_DIRECTORIES(${CMAKE_CURRENT_BINARY_DIR})
#
# src/include
#

INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR}/include)
#
# src/all
#

INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR}/all)

SET(SOURCES 
  all/all_allo.c
  )

#
# src/coloring
#

INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR}/coloring)
SET(SOURCES
	${SOURCES}
  coloring/coloring.c
  coloring/color_test.c
  coloring/g2l_hash.c
  coloring/bucket.c
  )

#
# src/graph
#

INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR}/graph)

SET(SOURCES
	${SOURCES}
  graph/graph.c
  )


#
# src/ha
#

INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR}/ha)
SET(SOURCES
	${SOURCES}
  ha/divide_machine.c
  ha/get_processor_name.c
  ha/ha_ovis.c
  )

#
# src/hier
#

INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR}/hier)

SET(SOURCES
	${SOURCES}
  hier/hier.c
  hier/hier_free_struct.c
  )

#
# src/hsfc
#

INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR}/hsfc)

SET(SOURCES
	${SOURCES}
  hsfc/hsfc_box_assign.c
  hsfc/hsfc.c
  hsfc/hsfc_hilbert.c
  hsfc/hsfc_point_assign.c
  )

#
# src/lb
#

INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR}/lb)

SET(SOURCES
	${SOURCES}
  lb/lb_balance.c
  lb/lb_box_assign.c
  lb/lb_copy.c
  lb/lb_eval.c
  lb/lb_free.c
  lb/lb_init.c
  lb/lb_invert.c
  lb/lb_migrate.c
  lb/lb_part2proc.c
  lb/lb_point_assign.c
  lb/lb_remap.c
  lb/lb_set_fn.c
  lb/lb_set_method.c
  lb/lb_set_part_sizes.c
  )

#
# src/matrix
#

INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR}/matrix)

SET(SOURCES
	${SOURCES}
  matrix/matrix_build.c
  matrix/matrix_distribute.c
  matrix/matrix_operations.c
  matrix/matrix_sym.c
  matrix/matrix_utils.c
  )

#
# src/order
#

INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR}/order)

SET(SOURCES
	${SOURCES}
  order/hsfcOrder.c
  order/order.c
  order/order_struct.c
  order/order_tools.c
  order/perm.c
  )

SET(SOURCES
	${SOURCES}
	#    order/hund.c
    )
#
# src/par
#

INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR}/par)

SET(SOURCES
	${SOURCES}
  par/par_average.c
  par/par_bisect.c
  par/par_median.c
  par/par_median_randomized.c
  par/par_stats.c
  par/par_sync.c
  par/par_tflops_special.c
  )

#
# src/params
#

INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR}/params)

SET(SOURCES
	${SOURCES}
  params/assign_param_vals.c
  params/bind_param.c
  params/check_param.c
  params/free_params.c
  params/key_params.c
  params/print_params.c
  params/set_param.c
  )



#
# src/tpls
#

INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR}/tpls)

SET(SOURCES
	${SOURCES}
  tpls/build_graph.c
  tpls/postprocessing.c
  tpls/preprocessing.c
  tpls/scatter_graph.c
  tpls/third_library.c
  tpls/verify_graph.c
  )

SET(SOURCES
	${SOURCES}
	#    tpls/parmetis_interface.c
    )

#
# src/phg
#

INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR}/phg)

SET(SOURCES
	${SOURCES}
  phg/phg_build.c
  phg/phg_build_calls.c
  phg/phg.c
  phg/phg_lookup.c
  phg/phg_verbose.c
  phg/phg_coarse.c
  phg/phg_comm.c
  phg/phg_distrib.c
  phg/phg_gather.c
  phg/phg_hypergraph.c
  phg/phg_match.c
  phg/phg_order.c
  phg/phg_parkway.c
  phg/phg_patoh.c
  phg/phg_plot.c
  phg/phg_rdivide.c
  phg/phg_refinement.c
  phg/phg_scale.c
  phg/phg_serialpartition.c
  phg/phg_tree.c
  phg/phg_two_ways.c
  phg/phg_util.c
  phg/phg_Vcycle.c
  )

#
# src/rcb
#

INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR}/rcb)

SET(SOURCES
	${SOURCES}
  rcb/box_assign.c
  rcb/create_proc_list.c
  rcb/inertial1d.c
  rcb/inertial2d.c
  rcb/inertial3d.c
  rcb/point_assign.c
  rcb/rcb_box.c
  rcb/rcb.c
  rcb/rcb_util.c
  rcb/rib.c
  rcb/rib_util.c
  rcb/shared.c
  )

#
# src/reftree
#

INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR}/reftree)

SET(SOURCES
	${SOURCES}
  reftree/reftree_build.c
  reftree/reftree_coarse_path.c
  reftree/reftree_hash.c
  reftree/reftree_part.c
  )

#
# src/simple
#

INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR}/simple)

SET(SOURCES
	${SOURCES}
  simple/block.c
  simple/cyclic.c
  simple/random.c
  )

#
# src/timer
#

INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR}/timer)

SET(SOURCES
	${SOURCES}
  timer/timer_params.c
  )

#
# src/Utilities
#

INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR}/Utilities/Communication)
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR}/Utilities/DDirectory)
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR}/Utilities/Timer)
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR}/Utilities/shared)

SET(SOURCES
	${SOURCES}
  Utilities/Communication/comm_exchange_sizes.c
  Utilities/Communication/comm_invert_map.c
  Utilities/Communication/comm_do.c
  Utilities/Communication/comm_do_reverse.c
  Utilities/Communication/comm_info.c
  Utilities/Communication/comm_create.c
  Utilities/Communication/comm_resize.c
  Utilities/Communication/comm_sort_ints.c
  Utilities/Communication/comm_destroy.c
  Utilities/Communication/comm_invert_plan.c
  Utilities/Timer/zoltan_timer.c
  Utilities/Timer/timer.c
  Utilities/DDirectory/DD_Find.c
  Utilities/DDirectory/DD_Destroy.c
  Utilities/DDirectory/DD_Set_Neighbor_Hash_Fn3.c
  Utilities/DDirectory/DD_Remove.c
  Utilities/DDirectory/DD_Create.c
  Utilities/DDirectory/DD_Update.c
  Utilities/DDirectory/DD_Stats.c
  Utilities/DDirectory/DD_Hash2.c
  Utilities/DDirectory/DD_Memory.c
  Utilities/DDirectory/DD_Print.c
  Utilities/DDirectory/DD_Set_Neighbor_Hash_Fn2.c
  Utilities/DDirectory/DD_Set_Hash_Fn.c
  Utilities/DDirectory/DD_Set_Neighbor_Hash_Fn1.c
  Utilities/Memory/mem.c
  Utilities/shared/zoltan_align.c
  Utilities/shared/zoltan_id.c
  )

#
# src/zz
#

INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR}/zz)

SET(SOURCES
	${SOURCES}
  zz/zz_coord.c
  zz/zz_gen_files.c
  zz/zz_hash.c
  zz/zz_map.c
  zz/zz_heap.c
  zz/zz_init.c
  zz/zz_obj_list.c
  zz/zz_rand.c
  zz/zz_set_fn.c
  zz/zz_sort.c
  zz/zz_struct.c
  zz/zz_back_trace.c
  zz/zz_util.c
  )

ADD_LIBRARY(zoltan ${SOURCES})
