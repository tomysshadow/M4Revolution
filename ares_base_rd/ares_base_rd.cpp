#include "ares_base_rd.h"

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

	Resource* Resource::Destroy(unsigned char flags) {
		return 0;
	}

	const char* Resource::GetClassNameA() const {
		return 0;
	}

	Resource* Resource::Clone(EnumCloneType enumCloneType) {
		return 0;
	}
}