
#include<string>
#include<unordered_map>

#include "communication/communication.h"
#include "utils/config.h"

#include "olz.h"
#include "../base/node.h"

#include "zoltan/zoltan_ghost_node_migrate.h"

using FPMAS::communication::MpiCommunicator;

namespace FPMAS {
	namespace graph {

		template<class T> class GhostNode;
		template<class T> class GhostArc;

		template<class T> class GhostGraph {
			friend GhostArc<T>;
			friend void zoltan::ghost::obj_size_multi_fn<T>(
				void *, int, int, int, ZOLTAN_ID_PTR, ZOLTAN_ID_PTR, int *, int *); 
			friend void zoltan::ghost::pack_obj_multi_fn<T>(
				void *, int, int, int, ZOLTAN_ID_PTR, ZOLTAN_ID_PTR, int *, int *, int *, char *, int *);
			friend void zoltan::ghost::unpack_obj_multi_fn<T>(
					void *, int, int, ZOLTAN_ID_PTR, int *, int *, char *, int *
					);
			private:
				MpiCommunicator mpiCommunicator;
				Zoltan* zoltan;

				std::set<unsigned long> importedNodeIds;

				std::unordered_map<unsigned long, std::string> ghost_node_serialization_cache;
				std::unordered_map<unsigned long, GhostNode<T>*> ghostNodes;
				std::unordered_map<unsigned long, GhostArc<T>*> ghostArcs;

				void link(GhostNode<T>*, Node<T>*, unsigned long);
				void link(Node<T>*, GhostNode<T>*, unsigned long);
				void link(GhostNode<T>*, GhostNode<T>*, unsigned long);

			public:
				GhostGraph(DistributedGraph<T>*);

				void synchronize();

				GhostNode<T>* buildNode(Node<T> node);

				std::unordered_map<unsigned long, GhostNode<T>*> getNodes();
				std::unordered_map<unsigned long, GhostArc<T>*> getArcs();

				void clearArc(Arc<T>*);

				~GhostGraph();

		};

		template<class T> GhostGraph<T>::GhostGraph(DistributedGraph<T>* origin) {
			this->zoltan = new Zoltan(mpiCommunicator.getMpiComm());
			FPMAS::config::zoltan_config(this->zoltan);

			zoltan->Set_Obj_Size_Multi_Fn(zoltan::ghost::obj_size_multi_fn<T>, origin);
			zoltan->Set_Pack_Obj_Multi_Fn(zoltan::ghost::pack_obj_multi_fn<T>, origin);
			zoltan->Set_Unpack_Obj_Multi_Fn(zoltan::ghost::unpack_obj_multi_fn<T>, origin);
		}

		template<class T> void GhostGraph<T>::synchronize() {
			int import_ghosts_num = this->ghostNodes.size(); 
			ZOLTAN_ID_PTR import_ghosts_global_ids = (ZOLTAN_ID_PTR) std::malloc(sizeof(unsigned int) * import_ghosts_num * 2);
			ZOLTAN_ID_PTR import_ghosts_local_ids;
			int* import_ghost_procs = (int*) std::malloc(sizeof(int) * import_ghosts_num);
			int* import_ghost_parts = (int*) std::malloc(sizeof(int) * import_ghosts_num);


			

		}

		/**
		 * Builds a GhostNode as an exact copy of the specified node.
		 *
		 * For each incoming/outgoing arcs of the input nodes, the
		 * corresponding source/target nodes are linked to the built GhostNode
		 * with GhostArc s. In other terms, all the arcs linked to the original
		 * node are duplicated.
		 *
		 * The original node can then be safely removed from the graph.
		 *
		 * @param node node from which a ghost copy must be created
		 * @param node_proc proc where the real node lives
		 */
		template<class T> GhostNode<T>* GhostGraph<T>::buildNode(Node<T> node) {
			// Copy the gNode from the original node, including arcs data
			GhostNode<T>* gNode = new GhostNode<T>(node);
			// Register the new GhostNode
			this->ghostNodes[gNode->getId()] = gNode;

			// Replaces the incomingArcs list by proper GhostArcs
			std::vector<Arc<T>*> temp_in_arcs = gNode->incomingArcs;
			gNode->incomingArcs.clear();	
			for(auto arc : temp_in_arcs) {
				Node<T>* localSourceNode = arc->getSourceNode();
				this->link(localSourceNode, gNode, arc->getId());
			}

			// Replaces the outgoingArcs list by proper GhostArcs
			std::vector<Arc<T>*> temp_out_arcs = gNode->outgoingArcs;
			gNode->outgoingArcs.clear();
			for(auto arc : temp_out_arcs) {
				Node<T>* localTargetNode = arc->getTargetNode();
				this->link(gNode, localTargetNode, arc->getId());
			}

			return gNode;

		}

		/**
		 * Links the specified nodes with a GhostArc.
		 */ 
		template<class T> void GhostGraph<T>::link(GhostNode<T>* source, Node<T>* target, unsigned long arc_id) {
			this->ghostArcs[arc_id] =
				new GhostArc<T>(arc_id, source, target);
		}

		/**
		 * Links the specified nodes with a GhostArc.
		 */ 
		template<class T> void GhostGraph<T>::link(Node<T>* source, GhostNode<T>* target, unsigned long arc_id) {
			this->ghostArcs[arc_id] =
				new GhostArc<T>(arc_id, source, target);
		}

		/**
		 * Links the specified nodes with a GhostArc.
		 */ 
		template<class T> void GhostGraph<T>::link(GhostNode<T>* source, GhostNode<T>* target, unsigned long arc_id) {
			this->ghostArcs[arc_id] =
				new GhostArc<T>(arc_id, source, target);
		}

		/**
		 * Returns the map of GhostNode s currently living in this graph.
		 *
		 * @return ghost nodes
		 */
		template<class T> std::unordered_map<unsigned long, GhostNode<T>*> GhostGraph<T>::getNodes() {
			return this->ghostNodes;
		}

		/**
		 * Returns the map of GhostArc s currently living in this graph.
		 *
		 * @return ghost arcs
		 */
		template<class T> std::unordered_map<unsigned long, GhostArc<T>*> GhostGraph<T>::getArcs() {
			return this->ghostArcs;
		}

		template<class T> void GhostGraph<T>::clearArc(Arc<T>* arc) {
			this->ghostArcs.erase(arc->getId());
			delete arc;
		}

		template<class T> GhostGraph<T>::~GhostGraph() {
			for(unsigned long id : this->importedNodeIds) {
				delete this->ghostNodes.at(id)->getData();
			}
			for(auto node : this->ghostNodes) {
				delete node.second;
			}
			for(auto arc : this->ghostArcs) {
				delete arc.second;
			}
			delete zoltan;
		}


	}
}
