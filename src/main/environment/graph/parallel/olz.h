#ifndef OLZ_H
#define OLZ_H

#include "../base/graph.h"
#include "olz_graph.h"

using FPMAS::graph::Node;

namespace FPMAS {
	namespace graph {
		/**
		 * GhostNode s are used to locally represent nodes that are living on
		 * an other processor, to maintain data continuity and the global graph
		 * structure.
		 *
		 * They are designed to be used as normal nodes in the graph structure.
		 * For example, when looking at nodes linked to a particular node, no
		 * difference can be made between nodes really living locally and ghost
		 * nodes.
		 */
		template <class T> class GhostNode : public Node<T> {

			friend GhostNode<T>* GhostGraph<T>::buildNode(unsigned long);
			friend GhostNode<T>* GhostGraph<T>::buildNode(Node<T>, std::set<unsigned long>);

			private:
				GhostNode(unsigned long);
				GhostNode(Node<T>);
		};

		template<class T> GhostNode<T>::GhostNode(unsigned long id)
			: Node<T>(id) {
			}

		/**
		 * Builds a GhostNode as a copy of the specified node.
		 *
		 * @param node original node
		 */
		template<class T> GhostNode<T>::GhostNode(Node<T> node)
			: Node<T>(node) {
			}

		/**
		 * GhostArcs should be used to link two nodes when at least one of the
		 * two nodes is a GhostNode.
		 *
		 * They behaves exactly as normal arcs. In consequence, when looking at
		 * a node outgoing arcs for example, no difference can directly be made
		 * between local arcs and ghost arcs.
		 *
		 */
		template<class T> class GhostArc : public Arc<T> {
			friend void GhostGraph<T>::link(GhostNode<T>*, Node<T>*, unsigned long);
			friend void GhostGraph<T>::link(Node<T>*, GhostNode<T>*, unsigned long);
			friend void GhostGraph<T>::link(GhostNode<T>*, GhostNode<T>*, unsigned long);

			private:
				GhostArc(unsigned long, GhostNode<T>*, Node<T>*);
				GhostArc(unsigned long, Node<T>*, GhostNode<T>*);
				GhostArc(unsigned long, GhostNode<T>*, GhostNode<T>*);

		};

		/**
		 * Builds a GhostArc linking the specified nodes. Notice that the
		 * GhostArc instance is added to the regular incoming arcs list of the
		 * local target node.
		 *
		 * @param arc_id arc id
		 * @param source pointer to the source ghost node
		 * @param target pointer to the local target node
		 */
		template<class T> GhostArc<T>::GhostArc(unsigned long arc_id, GhostNode<T>* source, Node<T>* target)
			: Arc<T>(arc_id, source, target) { };
		/**
		 * Builds a GhostArc linking the specified nodes. Notice that the
		 * GhostArc instance is added to the regular outgoing arcs list of the
		 * local source node.
		 *
		 * @param arc_id arc id
		 * @param source pointer to the local source node
		 * @param target pointer to the target ghost node
		 */
		template<class T> GhostArc<T>::GhostArc(unsigned long arc_id, Node<T>* source, GhostNode<T>* target)
			: Arc<T>(arc_id, source, target) { };

		/**
		 * Builds a GhostArc linking the specified ghost nodes.
		 *
		 * @param arc_id arc id
		 * @param source pointer to the source ghost node
		 * @param target pointer to the target ghost node
		 */
		template<class T> GhostArc<T>::GhostArc(unsigned long arc_id, GhostNode<T>* source, GhostNode<T>* target)
			: Arc<T>(arc_id, source, target) { };
	}
}

#endif
