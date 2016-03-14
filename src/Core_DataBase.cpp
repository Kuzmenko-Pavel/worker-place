#include "Core_DataBase.h"
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/join.hpp>
#include <vector>
#include <map>
#include <chrono>
#include "KompexSQLiteStatement.h"
#include "KompexSQLiteException.h"
#include "Config.h"
#include "Offer.h"
#include "../config.h"

#define CMD_SIZE 2621440

Core_DataBase::Core_DataBase():
    len(CMD_SIZE)
{
    cmd = new char[len];
}

Core_DataBase::~Core_DataBase()
{
    delete []cmd;
}

//-------------------------------------------------------------------------------------------------------------------
bool Core_DataBase::getOffers(Offer::Map &items, Params *_params, bool get_social)
{
    bool result = false;
    #ifdef DEBUG
        auto start = std::chrono::high_resolution_clock::now();
        printf("%s\n","------------------------------------------------------------------");
    #endif // DEBUG
    Kompex::SQLiteStatement *pStmt;
    params = _params;
    std::string offerSqlStr = "\
                    SELECT %s\
                    FROM Offer AS ofrs\
                    LEFT JOIN Offer2Rating AS oret ON ofrs.id=oret.id\
                    LEFT JOIN Informer2OfferRating AS iret ON iret.id_inf=%lld AND ofrs.id=iret.id_ofr\
                    %s %s %s ;\
                   ";
    std::string select_field ="\
        ofrs.id,\
        ofrs.campaignId,\
        ofrs.type,\
        ifnull(iret.rating,oret.rating) AS rating,\
        ofrs.UnicImpressionLot,\
        ofrs.height,\
        ofrs.width,\
        ofrs.isOnClick,\
        ofrs.social,\
        ofrs.account, \
        ofrs.offer_by_campaign_unique,\
        '',\
        '',\
        0,\
        ofrs.brending, \
        0,\
        ofrs.html_notification\
        ";
    std::string social = get_social ? "1" : "0";
    std::string cost = "";
    std::string gender = "";
    if (params->cost_.empty())
    {
        cost = " and cost = 0";
    }
    else
    {
        cost = " and cost IN (0," + params->cost_ + ")";
    }
    if (params->gender_.empty())
    {
        gender = " and gender = 0";
    }
    else
    {
        gender = " and gender IN (0," + params->gender_ + ")";
    }
    std::string where_offers = "WHERE ofrs.campaignId IN (SELECT id FROM "+ tmpTable->str() +" WHERE retargeting = 0 "+ cost + gender  +" ) AND ofrs.social = "+ social +"  AND ofrs.id NOT IN ( SELECT [offerId] FROM Session where id=" + params->getUserKey() +" and uniqueHits <= 0)"; 
    if (!params->getExcludedOffersString().empty())
    {
        where_offers = "WHERE ofrs.campaignId IN (SELECT id FROM "+ tmpTable->str() +" WHERE retargeting = 0 "+ cost + gender  +" ) AND ofrs.social = "+ social +"  AND ofrs.id NOT IN ("+ params->getExcludedOffersString() +")";
    }
    std::string order_offers = "ORDER BY rating DESC"; 
    std::string limit_offers = "LIMIT " + std::to_string(informer->capacity * 10); 

    bzero(cmd,sizeof(cmd));
    sqlite3_snprintf(len, cmd, offerSqlStr.c_str(),
                     select_field.c_str(),
                     informer->id,
                     where_offers.c_str(),
                     order_offers.c_str(),
                     limit_offers.c_str());
    try
    {
        pStmt = new Kompex::SQLiteStatement(cfg->pDb->pDatabase);

        try
        {
            pStmt->Sql(cmd);

            while(pStmt->FetchRow())
            {

                if(items.count(pStmt->GetColumnInt64(0)) > 0)
                {
                    continue;
                }

                Offer *off = new Offer(pStmt->GetColumnInt64(0),     //id
                                       pStmt->GetColumnInt64(1),    //campaignId
                                       pStmt->GetColumnInt(2),      //type
                                       pStmt->GetColumnDouble(3),   //rating
                                       pStmt->GetColumnInt(4),    //uniqueHits
                                       pStmt->GetColumnInt(5),     //height
                                       pStmt->GetColumnInt(6),     //width
                                       pStmt->GetColumnBool(7),    //isOnClick
                                       pStmt->GetColumnBool(8),    //social
                                       pStmt->GetColumnString(9),  //account_id
                                       pStmt->GetColumnInt(10),      //offer_by_campaign_unique
                                       pStmt->GetColumnString(11),   //recomendet
                                       pStmt->GetColumnString(12),   //retid
                                       "",
                                       pStmt->GetColumnBool(13),    // retargeting 
                                       pStmt->GetColumnBool(14),    //brending
                                       pStmt->GetColumnBool(15),    //isrecomendet
                                       pStmt->GetColumnBool(16)    //notification
                                      );
                items.insert(Offer::Pair(off->id_int,off));
                result = true;
            }
        }
        catch(Kompex::SQLiteException &ex)
        {
            std::clog<<"["<<pthread_self()<<"]"<<__func__<<" error: "
                     <<ex.GetString()
                     <<" \n"
                     <<cmd
                     <<params->get_.c_str()
                     <<params->post_.c_str()
                     <<std::endl;
        }

        pStmt->FreeQuery();
        delete pStmt;

    }
    catch(std::exception const &ex)
    {
        std::clog<<"["<<pthread_self()<<"]"<<__func__<<" error: "
                 <<ex.what()
                 <<" \n"
                 <<cmd
                 <<std::endl;
    }
    //clear if there is(are) banner
    #ifdef DEBUG
        auto elapsed = std::chrono::high_resolution_clock::now() - start;
        long long microseconds = std::chrono::duration_cast<std::chrono::microseconds>(elapsed).count();
        printf("Time %s taken: %lld \n", __func__,  microseconds);
        printf("%s\n","------------------------------------------------------------------");
    #endif // DEBUG
    return result;
}
