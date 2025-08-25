#pragma once


namespace utl {




	class ConfigSystem {
	public:
		static ConfigSystem& instance() {
			static ConfigSystem instance;
			return instance;
		}




	};

} // namespace utl



