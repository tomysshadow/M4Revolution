#include "Locale.h"

Locale& Locale::createGlobal(bool tryGlobal) {
	// if we aren't supposed to try the global locale, then throw
	if (!tryGlobal) {
		throw Invalid();
	}

	// otherwise, create the global locale
	name = getGlobalName();
	lc = LC_ALL;
	return create(false);
}

Locale& Locale::create(bool tryGlobal) {
	try {
		standardLocale = std::locale(getName(), lcToCategory(lc));
	} catch (std::runtime_error) {
		return createGlobal(tryGlobal);
	}

	cLocale = C_LOCALE(
		std::holds_alternative<std::wstring>(name)
		? _wcreate_locale(lc, std::get<std::wstring>(name).c_str())
		: _create_locale(lc, std::get<std::string>(name).c_str()),
	
		CLocaleDeleter()
	);

	if (!cLocale) {
		return createGlobal(tryGlobal);
	}
	return *this;
}

Locale& Locale::create(const NAME_VECTOR &nameVector, bool tryGlobal) {
	for (NAME_VECTOR::const_iterator nameVectorIterator = nameVector.begin(); nameVectorIterator != nameVector.end(); nameVectorIterator++) {
		name = *nameVectorIterator;

		try {
			create(false);
		} catch (Invalid) {
			continue;
		}
		return *this;
	}
	return createGlobal(tryGlobal);
}

void Locale::clear() {
	// clear the old locale first in case creating this fails with an exception
	standardLocale = {};

	if (cLocale) {
		cLocale = 0;
	}

	// we assume LC_ALL, as the copy constructor would, but setting the name is handled by the caller
	lc = LC_ALL;
}

const Locale::CATEGORY_LC_MAP Locale::categoryLCMap = {
	{std::locale::all, LC_ALL},
	{std::locale::collate, LC_COLLATE},
	{std::locale::ctype, LC_CTYPE},
	{std::locale::monetary, LC_MONETARY},
	{std::locale::numeric, LC_NUMERIC},
	{std::locale::time, LC_TIME}
};

const Locale::LC_CATEGORY_MAP Locale::lcCategoryMap = {
	{LC_ALL, std::locale::all},
	{LC_COLLATE, std::locale::collate},
	{LC_CTYPE, std::locale::ctype},
	{LC_MONETARY, std::locale::monetary},
	{LC_NUMERIC, std::locale::numeric},
	{LC_TIME, std::locale::time}
};

std::string Locale::getGlobalName() {
	char* globalName = setlocale(LC_ALL, 0);
	return globalName ? globalName : "C";
}

std::wstring Locale::getGlobalNameWide() {
	wchar_t* globalName = _wsetlocale(LC_ALL, 0);
	return globalName ? globalName : L"C";
}

Locale::LC Locale::categoryToLC(CATEGORY category) {
	// intentionally throws if value not in map
	return categoryLCMap.at(category & std::locale::all);
}

Locale::CATEGORY Locale::lcToCategory(LC lc) {
	// LC constant is not a bitmask so we can do a straight lookup with it
	LC_CATEGORY_MAP::const_iterator lcCategoryMapIterator = lcCategoryMap.find(lc);

	if (lcCategoryMapIterator == lcCategoryMap.end()) {
		return std::locale::none;
	}
	return lcCategoryMapIterator->second;
}

Locale::Locale() {
	createGlobal(true);
}

Locale::Locale(const char* name, LC lc, bool tryGlobal) : name(name ? name : getGlobalName()), lc(lc) {
	create(tryGlobal);
}

Locale::Locale(const wchar_t* name, LC lc, bool tryGlobal) : name(name ? name : getGlobalNameWide()), lc(lc) {
	create(tryGlobal);
}

Locale::Locale(const NAME &name, LC lc, bool tryGlobal) : name(name), lc(lc) {
	create(tryGlobal);
}

Locale::Locale(const NAME_VECTOR &nameVector, LC lc, bool tryGlobal) : lc(lc) {
	create(nameVector, tryGlobal);
}

Locale::Locale(const NAME_INITIALIZER_LIST &nameInitializerList, LC lc, bool tryGlobal) : lc(lc) {
	create(NAME_VECTOR(nameInitializerList), tryGlobal);
}

Locale::Locale(const std::string &copyString) {
	name = copyString;
	create(false);
}

Locale &Locale::operator=(const std::string &assignString) {
	clear();

	name = assignString;
	return create(false);
}

Locale::Locale(const NAME_VECTOR &copyNameVector) {
	create(copyNameVector, false);
}

Locale &Locale::operator=(const NAME_VECTOR &assignNameVector) {
	clear();
	return create(assignNameVector, false);
}

Locale::operator std::string() const {
	return getName();
}

Locale::operator std::wstring() const {
	return getNameWide();
}

Locale::operator std::locale() const {
	return standardLocale;
}

Locale::operator _locale_t() const {
	return cLocale.get();
}

std::string Locale::getName() const {
	// convert Wide to ANSI as necessary, assuming current codepage like _wcreate_locale does
	if (std::holds_alternative<std::wstring>(name)) {
		return std::string(CW2A(std::get<std::wstring>(name).c_str()));
	}
	return std::get<std::string>(name);
}

std::wstring Locale::getNameWide() const {
	if (std::holds_alternative<std::string>(name)) {
		return std::wstring(CA2W(std::get<std::string>(name).c_str()));
	}
	return std::get<std::wstring>(name).c_str();
}

std::locale Locale::getStandardLocale() const {
	return standardLocale;
}

Locale::CATEGORY Locale::getCategory() const {
	return lcToCategory(lc);
}

_locale_t Locale::getCLocale() const {
	return cLocale.get();
}

Locale::LC Locale::getLC() const {
	return lc;
}