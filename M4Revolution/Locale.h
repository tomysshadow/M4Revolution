#pragma once
#include <variant>
#include <string>
#include <vector>
#include <memory>
#include <type_traits>
#include <unordered_map>
#include <locale>
#include <stdexcept>
#include <locale.h>
#include <wchar.h>

#ifdef _WIN32
#include <windows.h>
#include <cguid.h>
#include <atlbase.h>
#include <atlconv.h>
#endif

class Locale {
	public:
	using Name = std::variant<std::string, std::wstring>;
	using NameVector = std::vector<Name>;
	using NameInitializerList = std::initializer_list<Name>;
	using Category = int;
	using Lc = int;

	private:
	#ifdef _WIN32
	using CLocale = std::shared_ptr<std::remove_pointer_t<_locale_t>>;

	struct CLocaleDeleter {
		void operator()(_locale_t localePointer) {
			if (localePointer) {
				_free_locale(localePointer);
			}
		}
	};
	#endif

	using CategoryLcMap = std::unordered_map<Category, Lc>;
	using LcCategoryMap = std::unordered_map<Lc, Category>;

	Locale& createGlobal(bool tryGlobal);
	Locale& create(bool tryGlobal);
	Locale& create(const NameVector &nameVector, bool tryGlobal);
	void clear();

	Name name = "C";

	std::locale standardLocale = {};

	CLocale cLocale = 0;
	Lc lc = LC_ALL;

	static std::string getGlobalName();
	static std::wstring getGlobalNameWide();
	public:
	static Lc categoryToLc(Category category);
	static Category lcToCategory(Lc lc);

	class Invalid : public std::invalid_argument {
		public:
		Invalid() noexcept : std::invalid_argument("Locale invalid") {
		}
	};

	Locale();
	explicit Locale(const Name &name, Lc lc = LC_ALL, bool tryGlobal = false);
	Locale(const NameVector &nameVector, Lc lc = LC_ALL, bool tryGlobal = false);
	Locale(const NameInitializerList &nameInitializerList, Lc lc = LC_ALL, bool tryGlobal = false);
	Locale(const char* name, Lc lc = LC_ALL, bool tryGlobal = false);
	Locale(const wchar_t* name, Lc lc = LC_ALL, bool tryGlobal = false);
	Locale(const std::string &copyString);
	Locale &operator=(const std::string &assignString);
	Locale(const NameVector &copyNameVector);
	Locale &operator=(const NameVector &assignNameVector);
	operator std::string() const;
	operator std::wstring() const;
	operator std::locale() const;
	#ifdef _WIN32
	operator _locale_t() const;
	#endif
	std::string getName() const;
	std::wstring getNameWide() const;
	std::locale getStandardLocale() const;
	Category getCategory() const;
	#ifdef _WIN32
	_locale_t getCLocale() const;
	#endif
	Lc getLc() const;
};