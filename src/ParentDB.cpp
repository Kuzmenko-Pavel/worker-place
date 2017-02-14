#include <vector>
#include <boost/algorithm/string.hpp>
#include "../config.h"
#include <mongocxx/instance.hpp>
#include <mongocxx/client.hpp>
#include <mongocxx/read_preference.hpp>
#include <bsoncxx/json.hpp>
#include <bsoncxx/builder/basic/document.hpp>
#include <bsoncxx/builder/basic/kvp.hpp>
#include <mongocxx/options/find.hpp>
#include <bsoncxx/types.hpp>
#include <bsoncxx/types/value.hpp>
#include <bsoncxx/document/value.hpp>
#include <bsoncxx/document/view.hpp>
#include <KompexSQLiteStatement.h>

#include "ParentDB.h"
#include "Log.h"
#include "json.h"
#include "Config.h"
#include "Offer.h"

using bsoncxx::builder::basic::document;
using bsoncxx::builder::basic::sub_document;
using bsoncxx::builder::basic::kvp;
using mongocxx::options::find;
using mongocxx::read_preference;

mongocxx::instance instance{};

ParentDB::ParentDB()
{
    pdb = Config::Instance()->pDb->pDatabase;
}

ParentDB::~ParentDB()
{
    //dtor
}

void ParentDB::loadRating(const std::string &id)
{
    mongocxx::client conn{mongocxx::uri{cfg->mongo_main_url_}};
    conn.read_preference(read_preference(read_preference::read_mode::k_secondary_preferred));
    auto coll = conn[cfg->mongo_main_db_]["stats_daily.rating"];
    auto filter = document{};
    Kompex::SQLiteStatement *pStmt;
    if(!id.size())
    {
        filter.append(kvp("full_rating",
                         [](sub_document subdoc) {
                                subdoc.append(kvp("$exists", bsoncxx::types::b_bool{true}));
                                }));
    }
    else
    {
        filter.append(kvp("full_rating",
                    [](sub_document subdoc) {
                    subdoc.append(kvp("$exists", bsoncxx::types::b_bool{true}));
                    }),
                kvp("adv_int", id));
        pStmt = new Kompex::SQLiteStatement(pdb);
        bzero(buf,sizeof(buf));
        sqlite3_snprintf(sizeof(buf),buf,"DELETE FROM Informer2OfferRating WHERE id_inf=%s;",id.c_str());
        try
        {
            pStmt->SqlStatement(buf);
        }
        catch(Kompex::SQLiteException &ex)
        {
            logDb(ex);
        }
        pStmt->FreeQuery();
        delete pStmt;
    }
    
        auto cursor = coll.find(filter.view());
        pStmt = new Kompex::SQLiteStatement(pdb);
        unsigned int transCount = 0;
        int c = 0;
        pStmt->BeginTransaction();
        std::vector<std::string> items;
        try{
            for (auto &&doc : cursor)
            {
               items.push_back(bsoncxx::to_json(doc));
            }
            for(auto i : items) {
                nlohmann::json x = nlohmann::json::parse(i);
                long long guid_int = x["guid_int"].get<long long>();
                long long adv_int =  x["adv_int"].get<long long>();
                bzero(buf,sizeof(buf));
                sqlite3_snprintf(sizeof(buf),buf, "INSERT INTO Informer2OfferRating(id_inf,id_ofr,rating) VALUES('%lld','%lld','%f');", adv_int, guid_int, x["full_rating"].get<float>());
                try
                {
                    pStmt->SqlStatement(buf);
                    ++c;
                }
                catch(Kompex::SQLiteException &ex)
                {
                    logDb(ex);
                    #ifdef DEBUG
                    printf("%s\n","--------------------------------------------");
                    printf("%s\n","INSERT INTO Informer2OfferRating");
                    #endif // DEBUG
                }
                transCount++;
                if (transCount % 1000 == 0)
                {
                    pStmt->CommitTransaction();
                    pStmt->FreeQuery();
                    pStmt->BeginTransaction();
                }
        }
    }
    catch(std::exception const &ex)
    {
        std::clog<<"["<<pthread_self()<<"]"<<__func__<<" error: "
                 <<ex.what()
                 <<" \n"
                 <<std::endl;
    }
    pStmt->CommitTransaction();
    pStmt->FreeQuery();
    Log::info("Load inf-rating for %d offers",c);
    delete pStmt;
}

/** Загружает все товарные предложения из MongoDb */
void ParentDB::OfferRatingLoad(document &query)
{

   Kompex::SQLiteStatement *pStmt;
   mongocxx::client conn{mongocxx::uri{cfg->mongo_main_url_}};
   conn.read_preference(read_preference(read_preference::read_mode::k_secondary_preferred));
   auto coll = conn[cfg->mongo_main_db_]["offer"];
   auto cursor = coll.find(query.view());
   long long long_id = 0;
   pStmt = new Kompex::SQLiteStatement(pdb);
   std::vector<std::string> items;
   unsigned int transCount = 0;
   pStmt->BeginTransaction();
   try{
         for (auto &&doc : cursor)
         {
            items.push_back(bsoncxx::to_json(doc));
         }
         for(auto i : items) {
             nlohmann::json x = nlohmann::json::parse(i);

            std::string id = x["guid"].get<std::string>();
            if (id.empty())
            {
                continue;
            }
            long_id = x["guid_int"].get<long long>();
            bzero(buf,sizeof(buf));
            sqlite3_snprintf(sizeof(buf),buf,"SELECT id FROM Offer WHERE id=%lld;", long_id);
            bool find = false; 
            try
            {
                pStmt->Sql(buf);
                while(pStmt->FetchRow())
                {
                    find = true;
                    break;
                }
                pStmt->FreeQuery();
            }
            catch(Kompex::SQLiteException &ex)
            {
                logDb(ex);
            }
            if (find)
            {
                bzero(buf,sizeof(buf));
                sqlite3_snprintf(sizeof(buf),buf,
        "INSERT OR REPLACE INTO Offer2Rating (id, rating) VALUES(%llu,%f);",
                                 long_id,
                                 x["full_rating"].get<float>()
                                );

                try
                {
                    pStmt->SqlStatement(buf);
                }
                catch(Kompex::SQLiteException &ex)
                {
                    #ifdef DEBUG
                    printf("%s\n","--------------------------------------------");
                    printf("%s\n","INSERT OR REPLACE INTO Offer2Rating");
                    #endif // DEBUG
                    logDb(ex);
                    pStmt->FreeQuery();
                }
                transCount++;
                if (transCount % 1000 == 0)
                {
                    pStmt->CommitTransaction();
                    pStmt->FreeQuery();
                    pStmt->BeginTransaction();
                }
            }

        }
    }
    catch(std::exception const &ex)
    {
        std::clog<<"["<<pthread_self()<<"]"<<__func__<<" error: "
                 <<ex.what()
                 <<" \n"
                 <<std::endl;
    }


    pStmt->CommitTransaction();
    pStmt->FreeQuery();
    delete pStmt;
}

void ParentDB::OfferLoad(document &query, document &camp)
{
    Kompex::SQLiteStatement *pStmt;
    int i = 0,
    skipped = 0;
    
    pStmt = new Kompex::SQLiteStatement(pdb);
    //bsonobjects.clear();
    //mongo::BSONObj o = camp.getObjectField("showConditions");
    //mongo::BSONObj f = BSON("guid"<<1<<"image"<<1<<"swf"<<1<<"guid_int"<<1<<"RetargetingID"<<1<<"campaignId_int"<<1<<"campaignId"<<1<<"campaignTitle"<<1
    //        <<"image"<<1<<"uniqueHits"<<1<<"description"<<1
    //        <<"url"<<1<<"Recommended"<<1<<"title"<<1);
    //std::string campaignId = camp.getStringField("guid");
    //auto cursor = monga_main->query(cfg->mongo_main_db_ + ".offer", q_correct, 0, 0, &f);
    //try{
    //    unsigned int transCount = 0;
    //    pStmt->BeginTransaction();
    //    while (cursor->more())
    //    {
    //        mongo::BSONObj itv = cursor->next();
    //        bsonobjects.push_back(itv.copy());
    //    }
    //    x = bsonobjects.begin();
    //    while(x != bsonobjects.end()) {
    //        std::string id = (*x).getStringField("guid");
    //        if (id.empty())
    //        {
    //            skipped++;
    //            continue;
    //        }

    //        std::string image = (*x).getStringField("image");
    //        if (image.empty())
    //        {
    //            skipped++;
    //            x++;
    //            continue;
    //        }

    //        bzero(buf,sizeof(buf));
    //        sqlite3_snprintf(sizeof(buf),buf,
    //            "INSERT OR REPLACE INTO Offer (\
    //            id,\
    //            guid,\
    //            campaignId,\
    //            image,\
    //            uniqueHits,\
    //            description,\
    //            url,\
    //            title,\
    //            campaign_guid,\
    //            social,\
    //            offer_by_campaign_unique,\
    //            UnicImpressionLot,\
    //            html_notification\
    //            )\
    //            VALUES(\
    //                    %llu,\
    //                    '%q',\
    //                    %llu,\
    //                    '%q',\
    //                    %d,\
    //                    '%q',\
    //                    '%q',\
    //                    '%q',\
    //                    '%q',\
    //                    %d,\
    //                    %d,\
    //                    %d,\
    //                    %d);",
    //            (*x).getField("guid_int").numberLong(),
    //            id.c_str(),
    //            (*x).getField("campaignId_int").numberLong(),
    //            (*x).getStringField("image"),
    //            (*x).getIntField("uniqueHits"),
    //            (*x).getStringField("description"),
    //            (*x).getStringField("url"),
    //            (*x).getStringField("title"),
    //            campaignId.c_str(),
    //            camp.getBoolField("social") ? 1 : 0,
    //            o.hasField("offer_by_campaign_unique") ? o.getIntField("offer_by_campaign_unique") : 1,
    //            o.hasField("UnicImpressionLot") ? o.getIntField("UnicImpressionLot") : 1,
    //            o.getBoolField("html_notification") ? 1 : 0);
    //
    //        try
    //        {
    //            pStmt->SqlStatement(buf);
    //        }
    //        catch(Kompex::SQLiteException &ex)
    //        {
    //            logDb(ex);
    //            skipped++;
    //            pStmt->FreeQuery();
    //        }
    //        transCount++;
    //        i++;
    //        x++;
    //        if (transCount % 1000 == 0)
    //        {
    //            pStmt->CommitTransaction();
    //            pStmt->FreeQuery();
    //            pStmt->BeginTransaction();
    //        }

    //    }
    //    pStmt->CommitTransaction();
    //    pStmt->FreeQuery();
    //    bsonobjects.clear();
    //}
    //catch(std::exception const &ex)
    //{
    //    std::clog<<"["<<pthread_self()<<"]"<<__func__<<" error: "
    //             <<ex.what()
    //             <<" \n"
    //             <<std::endl;
    //}

    //pStmt->FreeQuery();
    delete pStmt;
    
    //mongo::Query q;
    //q = mongo::Query("{ \"campaignId\" : \""+campaignId+"\"}");
    //OfferRatingLoad(q);

    //Log::info("Loaded %d offers", i);
    //if (skipped)
    //    Log::warn("Offers with empty id or image skipped: %d", skipped);
}
void ParentDB::OfferRemove(const std::string &id)
{
    Kompex::SQLiteStatement *pStmt;

    if(id.empty())
    {
        return;
    }

    pStmt = new Kompex::SQLiteStatement(pdb);
    sqlite3_snprintf(sizeof(buf),buf,"DELETE FROM Offer WHERE campaign_guid='%q';",id.c_str());
    try
    {
        pStmt->SqlStatement(buf);
    }
    catch(Kompex::SQLiteException &ex)
    {
        logDb(ex);
    }

    pStmt->FreeQuery();

    delete pStmt;

    Log::info("offer %s removed",id.c_str());
}
//-------------------------------------------------------------------------------------------------------


void ParentDB::logDb(const Kompex::SQLiteException &ex) const
{
    std::clog<<"ParentDB::logDb error: "<<ex.GetString()<<std::endl;
    std::clog<<"ParentDB::logDb request: "<<buf<<std::endl;
    #ifdef DEBUG
    printf("%s\n",ex.GetString().c_str());
    printf("%s\n",buf);
    #endif // DEBUG
}
/** \brief  Закгрузка всех рекламных кампаний из базы данных  Mongo

 */
//==================================================================================
void ParentDB::CampaignLoad(const std::string &aCampaignId)
{
    //mongo::Query query;

    //if(!aCampaignId.empty())
    //{
    //    query = mongo::Query("{\"guid\":\""+ aCampaignId +"\", \"status\" : \"working\",\"showConditions.retargeting\":false}");
    //}
    //else
    //{
    //    query = mongo::Query("{\"status\" : \"working\",\"showConditions.retargeting\":false}");
    //}
    //CampaignLoad(query);
}
/** \brief  Закгрузка всех рекламных кампаний из базы данных  Mongo

 */
//==================================================================================
void ParentDB::CampaignLoad(document &query)
{
    //std::unique_ptr<mongo::DBClientCursor> cursor;
    //int i = 0;

    //cursor = monga_main->query(cfg->mongo_main_db_ +".campaign", q_correct);
    //try{
    //while (cursor->more())
    //{
    //    mongo::BSONObj x = cursor->next();
    //    std::string id = x.getStringField("guid");
    //    if (id.empty())
    //    {
    //        Log::warn("Campaign with empty id skipped");
    //        continue;
    //    }

    //    std::string status = x.getStringField("status");
    //    
    //    CampaignRemove(id);

    //    if (status != "working")
    //    {
    //        Log::info("Campaign is hold: %s", id.c_str());
    //        continue;
    //    }

    //    //------------------------Create CAMP-----------------------
    //    //Загрузили все предложения
    //    mongo::Query q;
    //    #ifdef DUMMY
    //        q = mongo::Query("{$and: [{ \"retargeting\" : false}, {\"type\" : \"teaser\"}, {\"campaignId\":\""+ id +"\"}]}");
    //    #else
    //        q = mongo::Query("{\"campaignId\" : \""+ id + "\"}");
    //    #endif // DUMMY
    //    OfferLoad(q, x);
    //    Log::info("Loaded campaign: %s", id.c_str());
    //    i++;

    //}//while
    //}
    //catch(std::exception const &ex)
    //{
    //    std::clog<<"["<<pthread_self()<<"]"<<__func__<<" error: "
    //             <<ex.what()
    //             <<" \n"
    //             <<std::endl;
    //}

    //Log::info("Loaded %d campaigns",i); 
}


void ParentDB::CampaignRemove(const std::string &CampaignId)
{
    if(CampaignId.empty())
    {
        return;
    }

    Kompex::SQLiteStatement *pStmt;
    pStmt = new Kompex::SQLiteStatement(pdb);
    bzero(buf,sizeof(buf));
    sqlite3_snprintf(sizeof(buf),buf,"DELETE FROM Offer WHERE campaign_guid='%s';",CampaignId.c_str());
    try
    {
        pStmt->SqlStatement(buf);
    }
    catch(Kompex::SQLiteException &ex)
    {
        logDb(ex);
    }

    pStmt->FreeQuery();

    delete pStmt;
}
bool ParentDB::ClearSession(bool clearAll)
{
    try
    {
        Kompex::SQLiteStatement *pStmt;

        pStmt = new Kompex::SQLiteStatement(pdb);

        if(clearAll)
        {
            sqlite3_snprintf(sizeof(buf),buf,"DELETE FROM Session;");
        }
        else
        {
            sqlite3_snprintf(sizeof(buf),buf,"DELETE FROM Session;");
        }

        pStmt->SqlStatement(buf);

        delete pStmt;
    }
    catch(Kompex::SQLiteException &ex)
    {
        logDb(ex);
        return false;
    }

    return true;
}
