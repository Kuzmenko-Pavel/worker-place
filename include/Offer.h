#ifndef OFFER_H
#define OFFER_H

#include <string>
#include <list>
#include <map>
#include <vector>
#include <chrono>

#include "Campaign.h"
#include "EBranch.h"

typedef long long			sphinx_int64_t;
typedef unsigned long long	sphinx_uint64_t;

/** \brief  Класс описывает рекламное предложение (например, товар или новость). */
class Offer
{
public:
    typedef std::map <const unsigned long,Offer*> Map;
    typedef std::multimap <const float,Offer*, std::greater<float>> MapRate;
    typedef std::map <const unsigned long,Offer*>::iterator it;
    typedef std::map <const unsigned long,Offer*>::const_iterator cit;
    typedef std::pair<const float,Offer*> PairRate;
    typedef std::pair<const unsigned long,Offer*> Pair;
    typedef std::vector <Offer*> Vector;
    typedef std::vector <Offer*>::iterator itV;

    typedef enum { banner, teazer, unknown } Type;
    /// Структура для хранения информации о рекламном предложении.
    std::string id;             ///< ID предложения
    unsigned long long id_int;                 ///< ID предложения
    std::string title;          ///< Заголовок
    std::string description;    ///< Описание
    std::string url;            ///< URL перехода на предложение
    std::string image_url;      ///< URL картинки
    std::string swf;            ///< URL Flash
    unsigned long long campaign_id;    ///< ID кампании, к которой относится предложение
    Type type;			///< тип РП
    std::string conformity;		///< тип соответствия РП и запроса, изменяеться после выбора РП из индекса
    EBranchL branch;			///< ветка алгоритма по которой было выбрано РП
    std::string matching;       ///< Фраза соответствия
    float rating;				///< рейтинг РП
    int uniqueHits;		///< максимальное количество показов одному пользователю
    unsigned height;					///< высота РП (имеет значение для баннеров)
    unsigned width;					///< ширина РП (имеет значение для баннеров)
    bool isOnClick;             ///< Реклама по кликам или показам
    bool social;
    std::string campaign_guid;
    std::string account_id;
    std::string campaign_title;
    std::string project;
    unsigned unique_by_campaign;
    std::string Recommended;
    std::string retid;
    std::string retargeting_type;
    bool retargeting;
    bool brending;
    bool is_recommended;
    bool html_notification;                 

    bool loaded = false;
    long long int token_int = 0;
    std::chrono::milliseconds ms = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch());
    std::string token;          ///< Токен для проверки ссылки
    std::string redirect_url;   ///< Cсылка перенаправления
    unsigned showCount;
    unsigned long long category_id;

    //Offer(){};

    Offer(unsigned long long id_int,
          unsigned long long campaign_id,
          int type,
          float rating,
          int uniqueHits,
          int height,
          int width,
          bool isOnClick,
          bool social,
          std::string account_id,
          int unique_by_campaign,
          std::string Recommended,
          std::string retid,
          std::string retargeting_type,
          bool retargeting,
          bool brending,
          bool is_recommended,
          bool html_notification);

    virtual ~Offer();

    //bool operator==(const Offer &other) const { return this->id_int == other.id_int; }
    //bool operator<(const Offer &other) const { return rating < other.rating; }
    /*
        bool operator<(const Offer *x1, const Offer *x2)
        {
            return x1->rating > x2->rating;
        }
    */
    static Type typeFromString(const std::string &stype)
    {
        if(stype == "banner")
            return Type::banner;
        if(stype == "teaser")
            return Type::teazer;
        return Type::unknown;
    }

    static Type typeFromInt(int stype)
    {
        switch(stype)
        {
        case 0:
            return Type::banner;
        case 1:
            return Type::teazer;
        default:
            return Type::unknown;
        }
    }

    static std::string typeToString(const Type &stype)
    {
        switch(stype)
        {
            case Type::banner:
                return "banner";
            case Type::teazer:
                return "teaser";
            default:
                return "unknown";
        }
    }

    // Каждому элементу просмотра присваиваем уникальный токен
    void gen();
    void load();

    std::string toJson() const;
    bool setBranch(const  EBranchT tbranch);
    std::string getBranch() const
    {
        return EBranchL2String(branch);
    };
    private:
        char *cmd;
        size_t len;
};

#endif // OFFER_H

