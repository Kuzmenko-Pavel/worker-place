#include <sstream>

#include "Offer.h"
#include "Log.h"
#include "KompexSQLiteStatement.h"
#include "KompexSQLiteException.h"
#include "Config.h"
#include "json.h"
#include "../config.h"

#define CMD_SIZE 2621440

Offer::Offer(unsigned long long id_int,
             unsigned long long campaign_id,
             int type_int,
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
             bool html_notification
             ):
    id_int(id_int),
    campaign_id(campaign_id),
    type(typeFromInt(type_int)),
#ifndef DUMMY
    branch(EBranchL::L30),
#else
    branch(EBranchL::L0),
#endif
    len(CMD_SIZE),
    rating(rating),
    uniqueHits(uniqueHits),
    height(height),
    width(width),
    isOnClick(isOnClick),
    social(social),
    account_id(account_id),
    unique_by_campaign((unsigned)unique_by_campaign),
    Recommended(Recommended),
    retid(retid),
    retargeting_type(retargeting_type),
    retargeting(retargeting),
    brending(brending),
    is_recommended(is_recommended),
    html_notification(html_notification)
{
    cmd = new char[len];
}

Offer::~Offer()
{
    delete []cmd;
}

std::string Offer::toJson() const
{
    std::stringstream str_json;

    str_json << "{" <<
         "\"id\": \"" << id_int << "\"," <<
         "\"guid\": \"" << id << "\"," <<
         "\"title\": \"" << Json::Utils::Escape(title) << "\"," <<
         "\"description\": \"" << Json::Utils::Escape(description) << "\"," <<
         "\"price\": \"\",\n" <<
         "\"image\": \"" << Json::Utils::Escape(image_url) << "\"," <<
         "\"swf\": \"" << Json::Utils::Escape(swf) << "\"," <<
         "\"url\": \"" << Json::Utils::Escape(redirect_url) << "\"," <<
         "\"token\": \"" << token << "\"," <<
         "\"rating\": \"" << rating << "\"," <<
         "\"width\": \"" << width << "\"," <<
         "\"height\": \"" << height << "\"," <<
         "\"campaign_id\": \"" << campaign_id << "\"," <<
         "\"campaign_guid\": \"" << campaign_guid << "\"," <<
         "\"campaign_title\": \"" << campaign_title << "\"," <<
         "\"unique\": \"" << uniqueHits << "\"," <<
         "\"retargeting\": \"" << retargeting << "\"," <<
         "\"retargeting_type\": \"" << retargeting_type << "\"," <<
         "\"is_recommended\": \"" << is_recommended << "\"," <<
         "\"html_notification\": \"" << html_notification << "\"," <<
         "\"retid\": \"" << retid << "\"," <<
         "\"branch\": \"" << getBranch() << "\"" <<
         "}";

    return str_json.str();
}

void Offer::gen()
{
    if(token_int == 0)
    {
        token_int = (long long int)rand();
        ms = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch());
    }
    token = std::to_string(token_int) + std::to_string(ms.count()) + std::to_string(id_int);
}

void Offer::load()
{
    if(!loaded)
    {
        loaded = true;
        Kompex::SQLiteStatement *pStmt;
        std::string offerSqlStr;
        bzero(cmd,sizeof(cmd));
        offerSqlStr = "\
                        SELECT \
                        ofrs.guid,\
                        ofrs.title,\
                        ofrs.description,\
                        ofrs.url,\
                        ofrs.image,\
                        ofrs.swf,\
                        ofrs.campaign_guid,\
                        ofrs.campaign_title,\
                        ofrs.project\
                        FROM Offer AS ofrs\
                        where ofrs.id= %llu;\
                       ";
        sqlite3_snprintf(CMD_SIZE, cmd, offerSqlStr.c_str(), id_int);
        try
        {
            pStmt = new Kompex::SQLiteStatement(cfg->pDb->pDatabase);
            pStmt->Sql(cmd);
            while(pStmt->FetchRow())
            {
                id = pStmt->GetColumnString(0);
                title = pStmt->GetColumnString(1);
                description = pStmt->GetColumnString(2);
                url = pStmt->GetColumnString(3);
                image_url = pStmt->GetColumnString(4);
                swf = pStmt->GetColumnString(5);
                campaign_guid = pStmt->GetColumnString(6);
                campaign_title = pStmt->GetColumnString(7);
                project = pStmt->GetColumnString(8);
                break;
            }
        }
        catch(Kompex::SQLiteException &ex)
        {
            std::clog<<"["<<pthread_self()<<"]"<<__func__<<" error: "
                     <<ex.GetString()
                     <<" \n"
                     <<cmd
                     <<std::endl;
        }
        pStmt->FreeQuery();
        delete pStmt;

    }
}


bool Offer::setBranch(const EBranchT tbranch)
{
    switch(tbranch)
    {
    case EBranchT::T1:
        switch(type)
        {
        case Type::banner:
            switch(isOnClick)
            {
            case true:
                branch = EBranchL::L7;
                return true;
            case false:
                branch = EBranchL::L2;
                rating = 1000 * rating;
                return true;
            }
            break;
        case Type::teazer:
            switch(isOnClick)
            {
            case true:
                branch = EBranchL::L17;
                return true;
            case false:
                branch = EBranchL::L12;
                rating = 1000 * rating;
                return true;
            }
            break;
        default:
            return false;
        }
        break;
    case EBranchT::T2:
        switch(type)
        {
        case Type::banner:
            switch(isOnClick)
            {
            case true:
                branch = EBranchL::L8;
                return true;
            case false:
                branch = EBranchL::L3;
                rating = 1000 * rating;
                return true;
            }
            break;
        case Type::teazer:
            switch(isOnClick)
            {
            case true:
                branch = EBranchL::L18;
                return true;
            case false:
                branch = EBranchL::L13;
                return true;
            }
            break;
        default:
            return false;
        }
        break;
    case EBranchT::T3:
        switch(type)
        {
        case Type::banner:
            switch(isOnClick)
            {
            case true:
                branch = EBranchL::L4;
                rating = 1000 * rating;
                return true;
            case false:
                branch = EBranchL::L3;
                rating = 1000 * rating;
                return true;
            }
            break;
        case Type::teazer:
            switch(isOnClick)
            {
            case true:
                branch = EBranchL::L19;
                return true;
            case false:
                branch = EBranchL::L14;
                return true;
            }
            break;
        default:
            return false;
        }
        break;
    case EBranchT::T4:
        switch(type)
        {
        case Type::banner:
            switch(isOnClick)
            {
            case true:
                branch = EBranchL::L10;
                return true;
            case false:
                branch = EBranchL::L5;
                rating = 1000 * rating;
                return true;
            }
            break;
        case Type::teazer:
            switch(isOnClick)
            {
            case true:
                branch = EBranchL::L20;
                return true;
            case false:
                branch = EBranchL::L15;
                return true;
            }
            break;
        default:
            return false;
        }
        break;
    case EBranchT::T5:
        switch(type)
        {
        case Type::banner:
            switch(isOnClick)
            {
            case true:
                branch = EBranchL::L11;
                return true;
            case false:
                branch = EBranchL::L6;
                rating = 1000 * rating;
                return true;
            }
            break;
        case Type::teazer:
            switch(isOnClick)
            {
            case true:
                branch = EBranchL::L21;
                return true;
            case false:
                branch = EBranchL::L16;
                return true;
            }
            break;
        default:
            return false;
        }
        break;
    case EBranchT::TMAX:
        return false;
    }

    return false;
}

