#pragma once
#include <variant>
#include <string>
#include <vector>
#include <memory>
#include <type_traits>
#include <map>
#include <locale>
#include <stdexcept>
#include <locale.h>
#include <wchar.h>
#include <windows.h>
#include <cguid.h>
#include <atlbase.h>
#include <atlconv.h>

class Locale {
	public:
	typedef std::variant<std::string, std::wstring> NAME;
	typedef std::vector<NAME> NAME_VECTOR;
	typedef std::initializer_list<NAME> NAME_INITIALIZER_LIST;
	typedef int CATEGORY;
	typedef int LC;

	private:
	typedef std::shared_ptr<std::remove_reference<decltype(*_locale_t())>::type> C_LOCALE;

	struct CLocaleDeleter {
		void operator()(_locale_t localePointer) {
			if (localePointer) {
				_free_locale(localePointer);
			}
		}
	};

	typedef std::map<CATEGORY, LC> CATEGORY_LC_MAP;
	typedef std::map<LC, CATEGORY> LC_CATEGORY_MAP;

	Locale& createGlobal(bool tryGlobal);
	Locale& create(bool tryGlobal);
	Locale& create(const NAME_VECTOR &nameVector, bool tryGlobal);
	void clear();

	NAME name = "C";

	std::locale standardLocale = {};

	C_LOCALE cLocale = 0;
	LC lc = LC_ALL;

	static const CATEGORY_LC_MAP categoryLCMap;
	static const LC_CATEGORY_MAP lcCategoryMap;

	static std::string getGlobalName();
	static std::wstring getGlobalNameWide();
	public:
	static LC categoryToLC(CATEGORY category);
	static CATEGORY lcToCategory(LC lc);

	class Invalid : public std::invalid_argument {
		public:
		Invalid() noexcept : std::invalid_argument("Locale invalid") {
		}
	};

	Locale();
	Locale(const char* name, LC lc = LC_ALL, bool tryGlobal = false);
	Locale(const wchar_t* name, LC lc = LC_ALL, bool tryGlobal = false);
	Locale(const NAME &name, LC lc = LC_ALL, bool tryGlobal = false);
	Locale(const NAME_VECTOR &nameVector, LC lc = LC_ALL, bool tryGlobal = false);
	Locale(const NAME_INITIALIZER_LIST &nameInitializerList, LC lc = LC_ALL, bool tryGlobal = false);
	Locale(const std::string &copyString);
	Locale &operator=(const std::string &assignString);
	Locale(const NAME_VECTOR &copyNameVector);
	Locale &operator=(const NAME_VECTOR &assignNameVector);
	operator std::string() const;
	operator std::wstring() const;
	operator std::locale() const;
	operator _locale_t() const;
	std::string getName() const;
	std::wstring getNameWide() const;
	std::locale getStandardLocale() const;
	CATEGORY getCategory() const;
	_locale_t getCLocale() const;
	LC getLC() const;
};