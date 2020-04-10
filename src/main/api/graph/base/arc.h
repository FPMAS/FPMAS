#ifndef ARC_API_H
#define ARC_API_H

#include "id.h"

namespace FPMAS::api::graph::base {

	typedef int LayerId;

	template<typename T, typename IdType> class Node;

	template<typename T, typename IdType>
	class Arc {
		public:
			typedef FPMAS::api::graph::base::Node<T, IdType> abstract_node_ptr;

			virtual IdType getId() const = 0;
			virtual LayerId getLayer() const = 0;

			virtual float getWeight() const = 0;
			virtual void setWeight(float weight) = 0;

			virtual Node<T, IdType>* const getSourceNode() const = 0;
			virtual Node<T, IdType>* const getTargetNode() const = 0;

			void unlink();

			virtual ~Arc() {};
	};

	template<typename T, typename IdType>
		void Arc<T, IdType>::unlink() {
			this->getSourceNode()->unlinkOut(this, this->getLayer());
			this->getTargetNode()->unlinkIn(this, this->getLayer());
		}

}
#endif
