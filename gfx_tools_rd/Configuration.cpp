#include "Configuration.h"

namespace gfx_tools {
	Configuration Configuration::ms_currentConfiguration = Configuration();

	Configuration::Configuration() {
	}

	void Configuration::Set(Configuration const &configuration) {
		ms_currentConfiguration = configuration;
	}
}