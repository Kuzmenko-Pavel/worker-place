#ifndef PARAMS_H
#define PARAMS_H

#include <sstream>
#include <string>
#include <boost/date_time.hpp>
#include <boost/algorithm/string/join.hpp>
#include <vector>
#include <map>

/** \brief Параметры, которые определяют показ рекламы */
class Params
{
public:
    bool newClient;
    bool test_mode_;
    bool json_;
    bool storage_;
    bool async_;
    std::string ip_;
    std::string cookie_id_;
    std::string location_;
    unsigned long long key_long;
    boost::posix_time::ptime time_;
    std::string informer_id_;
    std::string w_;
    std::string h_;
    std::string D_;
    std::string M_;
    std::string H_;
    std::string context_;
    std::string device_;
    std::string search_;
    std::string get_;
    std::string post_;
    std::string cost_;
    std::string gender_;
    std::map<std::string,int> cost_accounts_;
    std::map<std::string,int> gender_accounts_;

    Params();
    /// IP посетителя.
    Params &ip(const std::string &ip, const std::string &qip);
    /// ID посетителя, взятый из cookie
    Params &cookie_id(const std::string &cookie_id);
    /// ID информера.
    Params &informer_id(const std::string &informer_id);
    /** \brief  Двухбуквенный код страны посетителя.

        Если не задан, то страна будет определена по IP.

        Этот параметр используется служебными проверками работы информеров
        в разных странах и в обычной работе не должен устанавливаться.

        \see region()
        \see ip()
    */
    Params &country(const std::string &country);
    /** \brief  Гео-политическая область в написании MaxMind GeoCity.

        Если не задана, то при необходимости будет определена по IP.

        Вместе с параметром country() используется служебными проверками
        работы информеров в разных странах и в обычной работе не должен
        устанавливаться.

        \see country()
        \see ip()
    */
    Params &region(const std::string &region);
    /** \brief  Тестовый режим работы, в котором показы не записываются и переходы не записываються.
        По умолчанию равен false.
    */
    Params &test_mode(bool test_mode);
    /// Выводить ли предложения в формате json.
    Params &json(bool json);
    /// Использовать пользовательское хранилише.
    Params &storage(bool storage_);
    Params &async(bool async_);
    /// ID предложений, которые следует исключить из показа.
    Params &excluded_offers(const std::vector<std::string> &excluded);
    Params &retargeting_exclude_offers(const std::vector<std::string> &retargeting_exclude);
    Params &retargeting_account_exclude_offers(const std::vector<std::string> &retargeting_exclude);
    Params &retargeting_view_offers(std::map<const unsigned long, int> &retargeting_view);
    /** \brief  Виртуальный путь и имя вызываемого скрипта.

        Используется для построения ссылок на самого себя. Фактически,
        сюда должна передаваться сервреная переменная SCRIPT_NAME.
    */
    Params &script_name(const char *script_name);
    /** \brief  Адрес страницы, на которой показывается информер.

        Обычно передаётся javascript загрузчиком.
    */
    Params &location(const std::string &location);
    Params &w(const std::string &w);
    Params &h(const std::string &h);
    Params &D(const std::string &D);
    Params &M(const std::string &M);
    Params &H(const std::string &H);
    Params &cost(const std::string &cost);
    Params &gender(const std::string &gender);
    Params &cost_accounts(const std::string &cost_accounts);
    Params &gender_accounts(const std::string &gender_accounts);
    /**
     * строка, содержашяя контекст страницы
     */
    Params &context(const std::string &context);
    /**
     * строка, содержашяя предыдуший контекст страницы
     */
    Params &context_short(const std::string &context_short);
    /**
     * строка, содержашяя поисковый запрос
     */
    Params &search(const std::string &search);
    Params &get(const std::string &get);
    Params &post(const std::string &post);
    
    Params &device(const std::string &device);
    /**
     * строка, содержашяя предыдушие поисковый запрос
     */
    Params &search_short(const std::string &search_short);
    /**
     * строка, содержашяя долгосрочную историю
     */
    Params &long_history(const std::string &long_history);
    /**
     * строка, содержашяя ретаргетинговую историю
     */
    Params &retargeting_offers(const std::vector<std::string> &retargeting);
    //*********************************************************************************************//
    std::string getIP() const;
    std::string getCookieId() const;
    std::string getUserKey() const;
    unsigned long long getUserKeyLong() const;
    std::string getCountry() const;
    std::string getRegion() const;
    std::string getInformerId() const;
    boost::posix_time::ptime getTime() const;
    bool isTestMode() const;
    bool isJson() const;
    bool isStorage() const;
    bool isAsync() const;
    std::vector<std::string> getExcludedOffers();
    std::vector<std::string> getRetargetingExcludedOffers();
    std::map<const unsigned long,int> getRetargetingViewOffers();
    std::map<const unsigned long,int> getRetargetingOffersDayMap();
    std::map<const unsigned long,bool> getRetargetingExcludedOffersMap();
    std::vector<std::string> getRetargetingOffers();
    std::vector<std::string> getRetargetingOffersId();
    std::string getExcludedOffersString();
    std::string getRetargetingExcludedOffersString();
    std::string getAccountRetargetingExcludedOffersString();
    std::string getScriptName() const;
    std::string getLocation() const;
    std::string getW() const;
    std::string getH() const;
    std::string getContext() const;
    std::string getSearch() const;
    std::string getDevice() const;
    std::string getUrl() const;

private:
    std::string country_;
    std::string countryByIp_;
    std::string region_;
    std::vector<std::string> excluded_offers_;
    std::vector<std::string> retargeting_exclude_offers_;
    std::vector<std::string> retargeting_account_exclude_offers_;
    std::map<const unsigned long,bool> retargeting_exclude_offers_map_;
    std::map<const unsigned long,int> retargeting_view_offers_;
    std::map<const unsigned long,int> retargeting_offers_day_;
    std::vector<std::string> retargeting_offers_;
    std::string script_name_;
    std::string context_short_;
    std::string search_short_;
    std::string long_history_;
};

#endif // PARAMS_H
