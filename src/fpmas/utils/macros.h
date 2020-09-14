#ifndef FPMAS_MACROS_H
#define FPMAS_MACROS_H

/** \file src/fpmas/utils/macros.h
 * FPMAS macros.
 */

/**
 * Converts arg to a C-like null terminated string.
 *
 * arg **must** be implicitly convertible to std::string
 *
 * This can be used to easily log thinks using FPMAS_LOG* macros.
 *
 * \par Example
 * ```cpp
 * DistributedId id {0, 4};
 * FPMAS_LOGI(RANK, "Tag", "%s", FPMAS_C_STR(id));
 * ```
 *
 * @param arg object to convert as a C-like string
 */
#define FPMAS_C_STR(arg) ((std::string) arg).c_str()

/**
 * MPI type that can be used to transmit fpmas::api::graph::DistributedId
 * instances.
 */
#define MPI_DISTRIBUTED_ID_TYPE \
	fpmas::api::graph::DistributedId::mpiDistributedIdType

/**
 * Elegant expression to specify a portion of code that should be executed only
 * on the process with the given rank.
 *
 * @param COMM reference to an fpmas::api::communication::MpiCommunicator
 * instance
 * @param RANK rank of the process on which the code should be executed
 */
#define FPMAS_ON_PROC(COMM, RANK) if(COMM.getRank() == RANK)

/**
 * Utility macro to easily build tasks bound to a node.
 *
 * @param node node to bind to the task
 * @param body function body run by the task
 */
#define FPMAS_NODE_TASK(node, body)\
	fpmas::scheduler::LambdaTask\
		<std::remove_pointer<decltype(node)>::type::data_type>(node, [&] () body)

#define FPMAS_DEFINE_GROUPS(...)\
	enum : fpmas::api::model::GroupId {\
		__VA_ARGS__ };

#define FPMAS_DEFINE_LAYERS(...)\
	enum : fpmas::api::graph::LayerId {\
		__VA_ARGS__ };

#endif
