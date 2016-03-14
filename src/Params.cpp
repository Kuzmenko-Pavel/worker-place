#include <sstream>

#include <boost/algorithm/string/trim.hpp>
#include <boost/algorithm/string/join.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/regex.hpp>
#include <boost/regex/icu.hpp>
#include <boost/date_time.hpp>

#include <string>

#include "Params.h"
#include "GeoIPTools.h"
#include "Log.h"
#include <map>


Params::Params() :
    newClient(false),
    test_mode_(false),
    json_(false),
    storage_(false),
    async_(false)
{
    time_ = boost::posix_time::second_clock::local_time();
}

/// IP посетителя.
Params &Params::ip(const std::string &ip, const std::string &qip)
{
    ip_ = ip;
    if(!qip.empty())
    {
        ip_ = qip;
    }

    country_ = geoip->country_code_by_addr(ip_);
    if (country_ == "NOT FOUND")
        region_ = "*";
    region_ = geoip->region_code_by_addr(ip_);

    return *this;
}

std::string time_t_to_string(time_t t)
{
    std::stringstream sstr;
    sstr << t;
    return sstr.str();
}

/// ID посетителя, взятый из cookie
Params &Params::cookie_id(const std::string &cookie_id)
{
    if(cookie_id.empty())
    {
        cookie_id_ = time_t_to_string(time(NULL));
        newClient = true;
    }
    else
    {
        cookie_id_ = cookie_id;
        boost::u32regex replaceSymbol = boost::make_u32regex("[^0-9]");
        cookie_id_ = boost::u32regex_replace(cookie_id_ ,replaceSymbol,"");
        newClient = false;
    }
    boost::trim(cookie_id_);
    key_long = atol(cookie_id_.c_str());

    return *this;
}

/// ID информера.
Params &Params::informer_id(const std::string &informer_id)
{
    informer_id_ = informer_id;
    boost::to_lower(informer_id_);
    return *this;
}
/** \brief  Двухбуквенный код страны посетителя.

    Если не задан, то страна будет определена по IP.

    Этот параметр используется служебными проверками работы информеров
    в разных странах и в обычной работе не должен устанавливаться.

    \see region()
    \see ip()
*/
Params &Params::country(const std::string &country)
{
    if(!country.empty())
    {
        country_ = country;
    }
    return *this;
}

/** \brief  Гео-политическая область в написании MaxMind GeoCity.

    Если не задана, то при необходимости будет определена по IP.

    Вместе с параметром country() используется служебными проверками
    работы информеров в разных странах и в обычной работе не должен
    устанавливаться.

    \see country()
    \see ip()
*/
Params &Params::region(const std::string &region)
{
    if(!region.empty())
    {
        region_ = region;
    }
    return *this;
}

Params &Params::device(const std::string &device)
{
    if(!device.empty())
    {
        device_ = device;
    }
    else
    {
        device_ = "pc";
    }
    return *this;
}
/** \brief  Тестовый режим работы, в котором показы не записываются и переходы не записываються.

    По умолчанию равен false.
*/
Params &Params::test_mode(bool test_mode)
{
    test_mode_ = test_mode;
    return *this;
}


/// Выводить ли предложения в формате json.
Params &Params::json(bool json)
{
    json_ = json;
    return *this;
}

/// Использовать ли пользовательское хранилище.
Params &Params::storage(bool storage)
{
    storage_ = storage;
    return *this;
}

/// Использовать ли пользовательское хранилище.
Params &Params::async(bool async)
{
    async_ = async;
    return *this;
}
/// ID предложений, которые следует исключить из показа.
Params &Params::excluded_offers(const std::vector<std::string> &excluded)
{
    excluded_offers_ = excluded;
    return *this;
}

Params &Params::retargeting_exclude_offers(const std::vector<std::string> &retargeting_exclude)
{
    retargeting_exclude_offers_ = retargeting_exclude;
    std::string::size_type sz;
    for (unsigned i=0; i<retargeting_exclude_offers_.size() ; i++)
    {
        try
        {
            long oi = std::stol (retargeting_exclude_offers_[i],&sz);
            retargeting_exclude_offers_map_.insert(std::pair<const unsigned long,bool>(oi,true));
        }
        catch (std::exception const &ex)
        {
            Log::err("exception %s: name: %s while processing retargeting_exclude_offers", typeid(ex).name(), ex.what(), retargeting_exclude_offers_[i].c_str());
        }
    }
    return *this;
}
Params &Params::retargeting_account_exclude_offers(const std::vector<std::string> &retargeting_exclude)
{
    retargeting_account_exclude_offers_ = retargeting_exclude;
    return *this;
}
Params &Params::retargeting_view_offers(std::map<const unsigned long, int> &retargeting_view)
{
    retargeting_view_offers_ = retargeting_view;
    return *this;
}

/// ID предложений, которые следует исключить из показа.
Params &Params::retargeting_offers(const std::vector<std::string> &retargeting)
{
    retargeting_offers_ = retargeting;
    std::string::size_type sz;
    for (auto i=retargeting_offers_.begin(); i != retargeting_offers_.end() ; ++i)
    {
        std::vector<std::string> par;
        boost::split(par, *i, boost::is_any_of("~"));
        if (!par.empty() && par.size() >= 4)
        {
            if (!par[0].empty())
            {
                try
                {
                    int od = std::stoi (par[3],&sz);
                    long oi = std::stol (par[0],&sz);
                    retargeting_offers_day_.insert(std::pair<const unsigned long,bool>(oi,od));
                }
                catch (std::exception const &ex)
                {
                    Log::err("exception %s: name: %s while processing etargeting_offers: %s", typeid(ex).name(), ex.what(), (*i).c_str());
                }
            }
        }
    }
    return *this;
}
/** \brief  Виртуальный путь и имя вызываемого скрипта.

    Используется для построения ссылок на самого себя. Фактически,
    сюда должна передаваться сервреная переменная SCRIPT_NAME.
*/
Params &Params::script_name(const char *script_name)
{
    script_name_ = script_name? script_name : "";
    return *this;
}

/** \brief  Адрес страницы, на которой показывается информер.

    Обычно передаётся javascript загрузчиком.
*/
Params &Params::location(const std::string &location)
{
    location_ = location;
    return *this;
}

Params &Params::w(const std::string &w)
{
    w_ = w;
    return *this;
}

Params &Params::h(const std::string &h)
{
    h_ = h;
    return *this;
}

Params &Params::D(const std::string &D)
{
    D_ = D;
    return *this;
}
Params &Params::M(const std::string &M)
{
    M_ = M;
    return *this;
}
Params &Params::H(const std::string &H)
{
    H_ = H;
    return *this;
}
Params &Params::cost(const std::string &cost)
{
    cost_ = cost;
    return *this;
}
Params &Params::gender(const std::string &gender)
{
    gender_ = gender;
    return *this;
}
Params &Params::cost_accounts(const std::string &cost_accounts)
{
    std::vector<std::string> cost_accounts_list;
    std::string::size_type sz;
    if(cost_accounts != "")
    {
        boost::split(cost_accounts_list, cost_accounts, boost::is_any_of(";"));
    }
    for (unsigned i=0; i<cost_accounts_list.size() ; i++)
    {
        std::vector<std::string> par;
        boost::split(par, cost_accounts_list[i], boost::is_any_of("~"));
        if (!par.empty() && par.size() >= 2)
        {
            try
            {
                std::string oi = par[0];
                int oc = std::stoi (par[1],&sz);
                cost_accounts_.insert(std::pair<std::string,int>(oi,oc));
            }
            catch (std::exception const &ex)
            {
                Log::err("exception %s: name: %s while processing cost_accounts", typeid(ex).name(), ex.what());
            }
        }
    }
    return *this;
}
Params &Params::gender_accounts(const std::string &gender_accounts)
{
    return *this;
}

/**
 * строка, содержашяя контекст страницы
 */
Params &Params::context(const std::string &context)
{
    context_ = context;
    return *this;
}
Params &Params::context_short(const std::string &context_short)
{
    context_short_ = context_short;
    return *this;
}
/**
 * строка, содержашяя поисковый запрос
 */
Params &Params::search(const std::string &search)
{
    search_ = search;
    return *this;
}
Params &Params::get(const std::string &get)
{
    get_ = get;
    return *this;
}
Params &Params::post(const std::string &post)
{
    post_ = post;
    return *this;
}
/**
 * строка, содержашяя поисковый запрос
 */
Params &Params::search_short(const std::string &search_short)
{
    search_short_ = search_short;
    return *this;
}
/**
 * строка, содержашяя поисковый запрос
 */
Params &Params::long_history(const std::string &long_history)
{
    long_history_ = long_history;
    return *this;
}

std::string Params::getIP() const
{
    return ip_;
}
std::string Params::getCookieId() const
{
    return cookie_id_;
}

std::string Params::getUserKey() const
{
    return cookie_id_;
}

unsigned long long Params::getUserKeyLong() const
{
    return key_long;
}

std::string Params::getCountry() const
{
    return country_;
}
std::string Params::getRegion() const
{
    return region_;
}

std::string Params::getInformerId() const
{
    return informer_id_;
}

boost::posix_time::ptime Params::getTime() const
{
    return time_;
}

bool Params::isTestMode() const
{
    return test_mode_;
}

bool Params::isJson() const
{
    return json_;
}

bool Params::isStorage() const
{
    return storage_;
}

bool Params::isAsync() const
{
    return async_;
}

std::vector<std::string> Params::getExcludedOffers()
{
    return excluded_offers_;
}

std::vector<std::string> Params::getRetargetingExcludedOffers()
{
    return retargeting_exclude_offers_;
}
std::map<const unsigned long,bool> Params::getRetargetingExcludedOffersMap()
{
    return retargeting_exclude_offers_map_;
}

std::map<const unsigned long,int> Params::getRetargetingViewOffers()
{
    return retargeting_view_offers_;
}
std::map<const unsigned long,int> Params::getRetargetingOffersDayMap()
{
    return retargeting_offers_day_;
}

std::vector<std::string> Params::getRetargetingOffers()
{
    return retargeting_offers_;
}

std::vector<std::string> Params::getRetargetingOffersId()
{
    std::vector<std::string> strs;
    for (unsigned i=0; i<retargeting_offers_.size() ; i++)
    {
        std::vector<std::string> par;
        boost::split(par, retargeting_offers_[i], boost::is_any_of("~"));
        if (!par.empty() && par.size() >= 3)
        {
            strs.push_back(par[0]);
        }
    }
    return strs;
}

std::string Params::getExcludedOffersString()
{
    return boost::algorithm::join(excluded_offers_, ", ");
}

std::string Params::getAccountRetargetingExcludedOffersString()
{
    return boost::algorithm::join(retargeting_account_exclude_offers_, ", ");
}

std::string Params::getRetargetingExcludedOffersString()
{
    return boost::algorithm::join(retargeting_exclude_offers_, ", ");
}

std::string Params::getScriptName() const
{
    return script_name_;
}

std::string Params::getLocation() const
{
    return location_;
}

std::string Params::getW() const
{
    return w_;
}

std::string Params::getH() const
{
    return h_;
}

std::string Params::getContext() const
{
    return context_;
}

std::string Params::getSearch() const
{
    return search_;
}

std::string Params::getDevice() const
{
    return device_;
}

std::string Params::getUrl() const
{
    std::stringstream url;

    url << script_name_ <<"?scr=" << informer_id_ <<"&show=json"<<"&w="<< w_<< "&h="<<h_<<"&device="<< device_<<"&country="<<country_<<"&region="<<region_<<"&ip="<<ip_;
    if (test_mode_)
    {
        url << "&test=true";
    }

    url << "&";

    return url.str();
}
