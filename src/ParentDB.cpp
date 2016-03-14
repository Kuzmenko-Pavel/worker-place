#include <vector>
#include <boost/algorithm/string.hpp>
#include "../config.h"

#include "ParentDB.h"
#include "Log.h"
#include "KompexSQLiteStatement.h"
#include "json.h"
#include "Config.h"
#include "Offer.h"

ParentDB::ParentDB()
{
    pdb = Config::Instance()->pDb->pDatabase;
    fConnectedToMainDatabase = false;
    ConnectMainDatabase();
}

ParentDB::~ParentDB()
{
    //dtor
}


bool ParentDB::ConnectMainDatabase()
{
    if(fConnectedToMainDatabase)
        return true;

    std::vector<mongo::HostAndPort> hvec;
    for(auto h = cfg->mongo_main_host_.begin(); h != cfg->mongo_main_host_.end(); ++h)
    {
        hvec.push_back(mongo::HostAndPort(*h));
        std::clog<<"Connecting to: "<<(*h)<<std::endl;
    }

    try
    {
        if(!cfg->mongo_main_set_.empty())
        {
            monga_main = new mongo::DBClientReplicaSet(cfg->mongo_main_set_, hvec);
            monga_main->connect();
        }


        if(!cfg->mongo_main_login_.empty())
        {
            std::string err;
            if(!monga_main->auth(cfg->mongo_main_db_,cfg->mongo_main_login_,cfg->mongo_main_passwd_, err))
            {
                std::clog<<"auth db: "<<cfg->mongo_main_db_<<" login: "<<cfg->mongo_main_login_<<" error: "<<err<<std::endl;
            }
            else
            {
                fConnectedToMainDatabase = true;
            }
        }
        else
        {
            fConnectedToMainDatabase = true;
        }
    }
    catch (mongo::UserException &ex)
    {
        std::clog<<"ParentDB::"<<__func__<<" mongo error: "<<ex.what()<<std::endl;
        return false;
    }

    return true;
}

void ParentDB::loadRating(const std::string &id)
{
    if(!fConnectedToMainDatabase)
        return;
    int c = 0;
    std::unique_ptr<mongo::DBClientCursor> cursor;
    std::vector<mongo::BSONObj> bsonobjects;
    std::vector<mongo::BSONObj>::const_iterator x;
    Kompex::SQLiteStatement *pStmt;
    pStmt = new Kompex::SQLiteStatement(pdb);
    mongo::BSONObj f = BSON("adv_int"<<1<<"guid_int"<<1<<"full_rating"<<1);
    mongo::Query query;
    if(!id.size())
    {
                query = mongo::Query("{\"full_rating\": {\"$exists\": true}}");
    }
    else
    {
                query =  mongo::Query("{\"adv_int\":"+ id +", \"full_rating\": {\"$exists\": true}}");

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
    }
    
    try{
        cursor = monga_main->query(cfg->mongo_main_db_ + ".stats_daily.rating", query, 0,0, &f);
        unsigned int transCount = 0;
        pStmt->BeginTransaction();
        while (cursor->more())
        {
            mongo::BSONObj itv = cursor->next();
            bsonobjects.push_back(itv.copy());
        }
        x = bsonobjects.begin();
        while(x != bsonobjects.end()) {
            long long guid_int = (*x).getField("guid_int").numberLong();
            long long adv_int = (*x).getField("adv_int").numberLong();
            bzero(buf,sizeof(buf));
            sqlite3_snprintf(sizeof(buf),buf, "INSERT INTO Informer2OfferRating(id_inf,id_ofr,rating) VALUES('%lld','%lld','%f');", adv_int, guid_int, (*x).getField("full_rating").numberDouble());
            try
            {
                pStmt->SqlStatement(buf);
                ++c;
            }
            catch(Kompex::SQLiteException &ex)
            {
                logDb(ex);
            }
            x++;
            transCount++;
            if (transCount % 1000 == 0)
            {
                pStmt->CommitTransaction();
                pStmt->FreeQuery();
                pStmt->BeginTransaction();
            }
        }
        bsonobjects.clear();
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
    Log::info("Load inf-rating for %d offers %s",c, query.toString().c_str());
    delete pStmt;
}

/** Загружает все товарные предложения из MongoDb */
void ParentDB::OfferRatingLoad(mongo::Query q_correct)
{
    if(!fConnectedToMainDatabase)
        return;

    Kompex::SQLiteStatement *pStmt;
    std::vector<mongo::BSONObj> bsonobjects;
    std::vector<mongo::BSONObj>::const_iterator x;
    mongo::BSONObj f = BSON("guid"<<1<<"guid_int"<<1<<"full_rating"<<1);
    auto cursor = monga_main->query(cfg->mongo_main_db_ + ".offer", q_correct, 0, 0, &f);

    pStmt = new Kompex::SQLiteStatement(pdb);

    try
    {
        unsigned int transCount = 0;
        pStmt->BeginTransaction();
        while (cursor->more())
        {
            mongo::BSONObj itv = cursor->next();
            bsonobjects.push_back(itv.copy());
        }
    x = bsonobjects.begin();
    while(x != bsonobjects.end()) {
            std::string id = (*x).getStringField("guid");
            if (id.empty())
            {
                continue;
            }

            bzero(buf,sizeof(buf));
            sqlite3_snprintf(sizeof(buf),buf,
    "INSERT OR REPLACE INTO Offer2Rating (id, rating) VALUES(%llu,%f);",
                             (*x).getField("guid_int").numberLong(),
                             (*x).getField("full_rating").numberDouble()
                            );

            try
            {
                pStmt->SqlStatement(buf);
            }
            catch(Kompex::SQLiteException &ex)
            {
                logDb(ex);
            }
            x++;
            transCount++;
            if (transCount % 1000 == 0)
            {
                pStmt->CommitTransaction();
                pStmt->FreeQuery();
                pStmt->BeginTransaction();
            }

        }
        bsonobjects.clear();
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

void ParentDB::OfferLoad(mongo::Query q_correct, const std::string &camp_id)
{
    if(!fConnectedToMainDatabase)
        return;
    Kompex::SQLiteStatement *pStmt;
    int i = 0,
    skipped = 0;

    pStmt = new Kompex::SQLiteStatement(pdb);
    
    sqlite3_snprintf(sizeof(buf),buf,"SELECT id, retargeting, project, social, offer_by_campaign_unique, account, target, UnicImpressionLot, recomendet_type, recomendet_count, brending, html_notification, retargeting_type FROM Campaign WHERE guid='%s' LIMIT 1;", camp_id.c_str());
    long long long_id = -1; 
    int ret = -1; 
    std::string project = "";
    int social = 0;
    int brending = 0;
    int html_notification = 0;
    int offer_by_campaign_unique = 1;
    std::string account = "";
    std::string target = "";
    int UnicImpressionLot = 1;
    std::string recomendet_type = "all";
    std::string retargeting_type = "offer";
    int recomendet_count = 10;
    try
    {
        pStmt->Sql(buf);
        while(pStmt->FetchRow())
        {
            long_id = pStmt->GetColumnInt64(0);
            ret = pStmt->GetColumnInt(1);
            project = pStmt->GetColumnString(2);
            social = pStmt->GetColumnInt(3);
            offer_by_campaign_unique = pStmt->GetColumnInt(4);
            account = pStmt->GetColumnString(5);
            target = pStmt->GetColumnString(6);
            UnicImpressionLot = pStmt->GetColumnInt(7);
            recomendet_type = pStmt->GetColumnString(8);
            recomendet_count = pStmt->GetColumnInt(9);
            brending = pStmt->GetColumnInt(10);
            html_notification = pStmt->GetColumnInt(11);
            retargeting_type = pStmt->GetColumnString(12);
            break;
        }
        pStmt->FreeQuery();
    }
    catch(Kompex::SQLiteException &ex)
    {
        logDb(ex);
    }
    bzero(buf,sizeof(buf));
    if (long_id != -1)
    {
        mongo::BSONObj f = BSON("guid"<<1<<"image"<<1<<"swf"<<1<<"guid_int"<<1<<"RetargetingID"<<1<<"campaignId_int"<<1<<"campaignId"<<1<<"campaignTitle"<<1
                <<"image"<<1<<"height"<<1<<"width"<<1<<"isOnClick"<<1<<"uniqueHits"<<1<<"description"<<1
                <<"url"<<1<<"Recommended"<<1<<"title"<<1<<"type"<<1);
        std::vector<mongo::BSONObj> bsonobjects;
        std::vector<mongo::BSONObj>::const_iterator x;
        auto cursor = monga_main->query(cfg->mongo_main_db_ + ".offer", q_correct, 0, 0, &f);
        try{
            unsigned int transCount = 0;
            pStmt->BeginTransaction();
            while (cursor->more())
            {
                mongo::BSONObj itv = cursor->next();
                bsonobjects.push_back(itv.copy());
            }
            x = bsonobjects.begin();
            while(x != bsonobjects.end()) {
                std::string id = (*x).getStringField("guid");
                if (id.empty())
                {
                    skipped++;
                    continue;
                }

                std::string image = (*x).getStringField("image");
                std::string swf = (*x).getStringField("swf");
                if (image.empty())
                {
                    if (swf.empty())
                    {
                        skipped++;
                        x++;
                        continue;
                    }
                }


                bzero(buf,sizeof(buf));
                if (ret == 1)
                {
                    if ( retargeting_type == "offer")
                    {
                        sqlite3_snprintf(sizeof(buf),buf,
                            "INSERT OR REPLACE INTO OfferR\
                            (id, guid, retid, campaignId, image, isOnClick, height, width, uniqueHits, swf, type, description, url, Recommended, title, campaign_guid, campaign_title, project, social, offer_by_campaign_unique,\
                             account, target, UnicImpressionLot, recomendet_type, recomendet_count, brending, html_notification)\
                            VALUES(%llu,'%q', '%q', %llu, '%q', %d, %d, %d, %d, '%q', %d, '%q', '%q', '%q', '%q','%q', '%q', '%q', %d, %d, '%q', '%q', %d, '%q', %d, %d, %d);",
                            (*x).getField("guid_int").numberLong(),
                            id.c_str(),
                            (*x).getStringField("RetargetingID"),
                            (*x).getField("campaignId_int").numberLong(),
                            (*x).getStringField("image"),
                            (*x).getBoolField("isOnClick") ? 1 : 0,
                            (*x).getIntField("height"),
                            (*x).getIntField("width"),
                            (*x).getIntField("uniqueHits"),
                            swf.c_str(),
                            Offer::typeFromString((*x).getStringField("type")),
                            (*x).getStringField("description"),
                            (*x).getStringField("url"),
                            (*x).getStringField("Recommended"),
                            (*x).getStringField("title"),
                            (*x).getStringField("campaignId"),
                            (*x).getStringField("campaignTitle"),
                            project.c_str(),
                            social,
                            offer_by_campaign_unique,
                            account.c_str(),
                            target.c_str(),
                            UnicImpressionLot,
                            recomendet_type.c_str(),
                            recomendet_count,
                            brending,
                            html_notification
                            );
                    }
                    else
                    {
                        sqlite3_snprintf(sizeof(buf),buf,
                            "INSERT OR REPLACE INTO OfferA\
                            (id, guid, campaignId, image, isOnClick, height, width, uniqueHits, swf, type, description, url, title, campaign_guid, campaign_title, project, social, offer_by_campaign_unique,\
                             account, UnicImpressionLot, html_notification)\
                            VALUES(%llu,'%q', %llu, '%q', %d, %d, %d, %d, '%q', %d, '%q', '%q', '%q', '%q','%q', '%q',  %d, %d, '%q', %d, %d);",
                            (*x).getField("guid_int").numberLong(),
                            id.c_str(),
                            (*x).getField("campaignId_int").numberLong(),
                            (*x).getStringField("image"),
                            (*x).getBoolField("isOnClick") ? 1 : 0,
                            (*x).getIntField("height"),
                            (*x).getIntField("width"),
                            (*x).getIntField("uniqueHits"),
                            swf.c_str(),
                            Offer::typeFromString((*x).getStringField("type")),
                            (*x).getStringField("description"),
                            (*x).getStringField("url"),
                            (*x).getStringField("title"),
                            (*x).getStringField("campaignId"),
                            (*x).getStringField("campaignTitle"),
                            project.c_str(),
                            social,
                            offer_by_campaign_unique,
                            account.c_str(),
                            UnicImpressionLot,
                            html_notification
                            );

                    }
                }
                else
                {
                    sqlite3_snprintf(sizeof(buf),buf,
                        "INSERT OR REPLACE INTO Offer\
                        (id, guid, campaignId, image, isOnClick, height, width, uniqueHits, swf, type, description, url, title, campaign_guid, campaign_title, project, social, offer_by_campaign_unique,\
                         account, UnicImpressionLot, brending, html_notification)\
                        VALUES(%llu,'%q',  %llu, '%q', %d, %d, %d, %d, '%q', %d, '%q', '%q', '%q', '%q','%q', '%q', %d, %d, '%q', %d, %d, %d);",
                        (*x).getField("guid_int").numberLong(),
                        id.c_str(),
                        (*x).getField("campaignId_int").numberLong(),
                        (*x).getStringField("image"),
                        (*x).getBoolField("isOnClick") ? 1 : 0,
                        (*x).getIntField("height"),
                        (*x).getIntField("width"),
                        (*x).getIntField("uniqueHits"),
                        swf.c_str(),
                        Offer::typeFromString((*x).getStringField("type")),
                        (*x).getStringField("description"),
                        (*x).getStringField("url"),
                        (*x).getStringField("title"),
                        (*x).getStringField("campaignId"),
                        (*x).getStringField("campaignTitle"),
                        project.c_str(),
                        social,
                        offer_by_campaign_unique,
                        account.c_str(),
                        UnicImpressionLot,
                        brending,
                        html_notification
                        );
                }
        
                try
                {
                    pStmt->SqlStatement(buf);
                }
                catch(Kompex::SQLiteException &ex)
                {
                    logDb(ex);
                    skipped++;
                }
                transCount++;
                i++;
                x++;
                if (transCount % 1000 == 0)
                {
                    pStmt->CommitTransaction();
                    pStmt->FreeQuery();
                    pStmt->BeginTransaction();
                }

            }
            pStmt->CommitTransaction();
            pStmt->FreeQuery();
            bsonobjects.clear();
        }
        catch(std::exception const &ex)
        {
            std::clog<<"["<<pthread_self()<<"]"<<__func__<<" error: "
                     <<ex.what()
                     <<" \n"
                     <<std::endl;
        }

    }

    pStmt->FreeQuery();
    delete pStmt;
    if (long_id != -1 && ret == 0)
    {
        mongo::Query q;
        q = mongo::Query("{ \"campaignId\" : \""+camp_id+"\"}");
        OfferRatingLoad(q);
    }

    Log::info("Loaded %d offers", i);
    if (skipped)
        Log::warn("Offers with empty id or image skipped: %d", skipped);
}
void ParentDB::OfferRemove(const std::string &id)
{
    Kompex::SQLiteStatement *pStmt;

    if(id.empty())
    {
        return;
    }

    pStmt = new Kompex::SQLiteStatement(pdb);
    sqlite3_snprintf(sizeof(buf),buf,"DELETE FROM Offer WHERE guid='%s';",id.c_str());
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
    mongo::Query query;

#ifdef DUMMY
    if(!aCampaignId.empty())
    {
        query = mongo::Query("{\"guid\":\""+ aCampaignId +"\", \"status\" : \"working\",\"showConditions.retargeting\":false}");
    }
    else
    {
        query = mongo::Query("{\"status\" : \"working\",\"showConditions.retargeting\":false}");
    }
#else
    if(!aCampaignId.empty())
    {
        query = mongo::Query("{\"guid\":\""+ aCampaignId +"\", \"status\" : \"working\"}");
    }
    else
    {
        query = mongo::Query("{\"status\" : \"working\"}");
    }
#endif
    CampaignLoad(query);
}
void ParentDB::CampaignLoadWhitOutRetargetingOnly(const std::string &aCampaignId)
{
    mongo::Query query;

#ifdef DUMMY
    if(!aCampaignId.empty())
    {
        query = mongo::Query("{\"guid\":\""+ aCampaignId +"\", \"status\" : \"working\",\"showConditions.retargeting\":false}");
    }
    else
    {
        query = mongo::Query("{\"status\" : \"working\",\"showConditions.retargeting\":false}");
    }
#else
    if(!aCampaignId.empty())
    {
        query = mongo::Query("{\"guid\":\""+ aCampaignId +"\", \"status\" : \"working\",\"showConditions.retargeting\":false}");
    }
    else
    {
        query = mongo::Query("{\"status\" : \"working\",\"showConditions.retargeting\":false}");
    }
#endif
    CampaignLoad(query);
}
void ParentDB::CampaignLoadRetargetingOnly(const std::string &aCampaignId)
{
    mongo::Query query;

#ifdef DUMMY
    if(!aCampaignId.empty())
    {
        query = mongo::Query("{\"guid\":\""+ aCampaignId +"\", \"status\" : \"working\",\"showConditions.retargeting\":false}");
    }
    else
    {
        query = mongo::Query("{\"status\" : \"working\",\"showConditions.retargeting\":false}");
    }
#else
    if(!aCampaignId.empty())
    {
        query = mongo::Query("{\"guid\":\""+ aCampaignId +"\", \"status\" : \"working\",\"showConditions.retargeting\":true}");
    }
    else
    {
        query = mongo::Query("{\"status\" : \"working\",\"showConditions.retargeting\":true}");
    }
#endif
    CampaignLoad(query);
}
/** \brief  Закгрузка всех рекламных кампаний из базы данных  Mongo

 */
//==================================================================================
void ParentDB::CampaignLoad(mongo::Query q_correct)
{
    std::unique_ptr<mongo::DBClientCursor> cursor;
    Kompex::SQLiteStatement *pStmt;
    int i = 0;

    cursor = monga_main->query(cfg->mongo_main_db_ +".campaign", q_correct);
    try{
    while (cursor->more())
    {
        pStmt = new Kompex::SQLiteStatement(pdb);
        bzero(buf,sizeof(buf));
        mongo::BSONObj x = cursor->next();
        std::string id = x.getStringField("guid");
        if (id.empty())
        {
            Log::warn("Campaign with empty id skipped");
            continue;
        }

        mongo::BSONObj o = x.getObjectField("showConditions");

        long long long_id = x.getField("guid_int").numberLong();
        std::string status = x.getStringField("status");

        showCoverage cType = Campaign::typeConv(o.getStringField("showCoverage"));

        //------------------------Clean-----------------------
        CampaignRemove(id);

        if (status != "working")
        {
            try
            {
                pStmt->SqlStatement(buf);
                pStmt->FreeQuery();
            }
            catch(Kompex::SQLiteException &ex)
            {
                logDb(ex);
            }
            delete pStmt;
            Log::info("Campaign is hold: %s", id.c_str());
            continue;
        }

        //------------------------Create CAMP-----------------------
        bzero(buf,sizeof(buf));
        Log::info(x.getStringField("account"));
        sqlite3_snprintf(sizeof(buf),buf,
                         "INSERT OR REPLACE INTO Campaign\
                         (id,guid,title,project,social,showCoverage,impressionsPerDayLimit,retargeting,recomendet_type,recomendet_count,account,target,offer_by_campaign_unique, UnicImpressionLot, brending, html_notification \
                          , retargeting_type, cost, gender) \
                         VALUES(%lld,'%q','%q','%q',%d,%d,%d,%d,'%q',%d,'%q','%q',%d,%d,%d, %d, '%q', %d, %d);",
                         long_id,
                         id.c_str(),
                         x.getStringField("title"),
                         x.getStringField("project"),
                         x.getBoolField("social") ? 1 : 0,
                         cType,
                         x.getField("impressionsPerDayLimit").numberInt(),
                         o.getBoolField("retargeting") ? 1 : 0,
                         o.hasField("recomendet_type") ? o.getStringField("recomendet_type") : "all",
                         o.hasField("recomendet_count") ? o.getIntField("recomendet_count") : 10,
                         x.getStringField("account"),
                         o.getStringField("target"),
                         o.hasField("offer_by_campaign_unique") ? o.getIntField("offer_by_campaign_unique") : 1,
                         o.hasField("UnicImpressionLot") ? o.getIntField("UnicImpressionLot") : 1,
                         o.getBoolField("brending") ? 1 : 0,
                         o.getBoolField("html_notification") ? 1 : 0,
                         o.hasField("retargeting_type") ? o.getStringField("retargeting_type") : "offer",
                         o.hasField("cost") ? o.getIntField("cost") : 0,
                         o.hasField("gender") ? o.getIntField("gender") : 0
                        );
        try
        {
            pStmt->SqlStatement(buf);
        }
        catch(Kompex::SQLiteException &ex)
        {
            logDb(ex);
        }

        //Загрузили все предложения
        mongo::Query q;
        #ifdef DUMMY
            q = mongo::Query("{$and: [{ \"retargeting\" : false}, {\"type\" : \"teaser\"}, {\"campaignId\":\""+ id +"\"}]}");
        #else
            q = mongo::Query("{\"campaignId\" : \""+ id + "\"}");
        #endif // DUMMY
        OfferLoad(q, id);
        Log::info("Loaded campaign: %s", id.c_str());
        i++;

    }//while
    }
    catch(std::exception const &ex)
    {
        std::clog<<"["<<pthread_self()<<"]"<<__func__<<" error: "
                 <<ex.what()
                 <<" \n"
                 <<std::endl;
    }

    Log::info("Loaded %d campaigns",i); 
}


void ParentDB::CampaignRemove(const std::string &aCampaignId)
{
    if(aCampaignId.empty())
    {
        return;
    }

    Kompex::SQLiteStatement *pStmt;

    pStmt = new Kompex::SQLiteStatement(pdb);
    
    bzero(buf,sizeof(buf));
    sqlite3_snprintf(sizeof(buf),buf,"SELECT id FROM Campaign WHERE guid='%s';",aCampaignId.c_str());
    long long long_id = -1; 
    try
    {
        pStmt->Sql(buf);
        while(pStmt->FetchRow())
        {
            long_id = pStmt->GetColumnInt64(0);
            break;
        }
        pStmt->FreeQuery();
    }
    catch(Kompex::SQLiteException &ex)
    {
        logDb(ex);
    }

    bzero(buf,sizeof(buf));
    sqlite3_snprintf(sizeof(buf),buf,"DELETE FROM Campaign WHERE id='%lld';",long_id);
    try
    {
        pStmt->SqlStatement(buf);
    }
    catch(Kompex::SQLiteException &ex)
    {
        logDb(ex);
    }
    bzero(buf,sizeof(buf));
  

    pStmt->FreeQuery();

    delete pStmt;
    
    if (long_id != -1)
    {
        Log::info("campaign %s removed",aCampaignId.c_str());
    }
    else
    {
        Log::info("campaign %s not removed, not found",aCampaignId.c_str());
    }
}

//==================================================================================
std::string ParentDB::CampaignGetName(long long campaign_id)
{
    auto cursor = monga_main->query(cfg->mongo_main_db_ +".campaign", QUERY("guid_int" << campaign_id));

    while (cursor->more())
    {
        mongo::BSONObj x = cursor->next();
        return x.getStringField("guid");
    }
    return "";
}
