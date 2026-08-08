#include "ares_base.h"

namespace ares {
	Resource::Resource() {
	}

	Resource::~Resource() {
	}

	Resource::Resource(Resource const &resource) {
	}

	Resource &Resource::operator=(Resource const &resource) {
		return *this;
	}

	const char* Resource::GetClassNameA() const {
		return nullptr;
	}

	Resource* Resource::Clone(EnumCloneType enumCloneType) {
		return nullptr;
	}
}