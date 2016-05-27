#include <boost/foreach.hpp>
#include <boost/date_time/posix_time/ptime.hpp>
#include <boost/date_time/posix_time/posix_time_io.hpp>
#include <boost/format.hpp>
#include <boost/algorithm/string.hpp>
#include <map>

#include <ctime>
#include <cstdlib>
#include <chrono>

#include "../config.h"

#include "Config.h"
#include "Core.h"
#include "DB.h"
#include "base64.h"

int HistoryManager::request_processed_ = 0;
int HistoryManager::offer_processed_ = 0;
int HistoryManager::social_processed_ = 0;

Core::Core()
{
    tid = pthread_self();

    hm = new HistoryManager();

    std::clog<<"["<<tid<<"]core start"<<std::endl;
}
//-------------------------------------------------------------------------------------------------------------------
Core::~Core()
{
    delete hm;
}
//-------------------------------------------------------------------------------------------------------------------
std::string Core::Process(Params *prms)
{
    #ifdef DEBUG
        auto start = std::chrono::high_resolution_clock::now();
        printf("%s\n","/////////////////////////////////////////////////////////////////////////");
    #endif // DEBUG
    startCoreTime = boost::posix_time::microsec_clock::local_time();

    params = prms;
    hm->startGetUserHistory(params);
    getOffers(items, params);
    RISAlgorithm(items);
    resultHtml();

    endCoreTime = boost::posix_time::microsec_clock::local_time();
    #ifdef DEBUG
        auto elapsed = std::chrono::high_resolution_clock::now() - start;
        long long microseconds = std::chrono::duration_cast<std::chrono::microseconds>(elapsed).count();
        printf("Time %s taken: %lld \n", __func__,  microseconds);
        printf("%s\n","/////////////////////////////////////////////////////////////////////////");
    #endif // DEBUG

    return retHtml;
}
//-------------------------------------------------------------------------------------------------------------------
void Core::log()
{
    if(cfg->toLog())
    {
        std::clog<<"["<<tid<<"]";
    }
    if(cfg->logCoreTime)
    {
        std::clog<<" core time:"<< boost::posix_time::to_simple_string(endCoreTime - startCoreTime);
    }

    if(cfg->logOutPutSize)
        std::clog<<" out:"<<vResult.size();

}
//-------------------------------------------------------------------------------------------------------------------
void Core::ProcessSaveResults()
{
    log();
    if (!params->isStorageMode())
    {
        hm->updateUserHistory(vResult);
    }

    for (Offer::it o = items.begin(); o != items.end(); ++o)
    {
        if(o->second)
            delete o->second;
    }
    //clear all offers map
    items.clear();
    result.clear();
    resultSocial.clear();
    vResult.clear();
    vResultSocial.clear();
    OutPutCampaignSet.clear();
    OutPutOfferSet.clear();
    OutPutSocialCampaignSet.clear();
    OutPutSocialOfferSet.clear();

    if(cfg->toLog())
        std::clog<<std::endl;
}
//-------------------------------------------------------------------------------------------------------------------
nlohmann::json Core::OffersToJson(Offer::Vector &items)
{
    #ifdef DEBUG
        auto start = std::chrono::high_resolution_clock::now();
        printf("%s\n","/////////////////////////////////////////////////////////////////////////");
    #endif // DEBUG
    nlohmann::json j;
    for (auto it = items.begin(); it != items.end(); ++it)
    {
        j.push_back((*it)->toJson());
    }
    #ifdef DEBUG
        auto elapsed = std::chrono::high_resolution_clock::now() - start;
        long long microseconds = std::chrono::duration_cast<std::chrono::microseconds>(elapsed).count();
        printf("Time %s taken: %lld \n", __func__,  microseconds);
        printf("%s\n","/////////////////////////////////////////////////////////////////////////");
    #endif // DEBUG
    return j;
}
//-------------------------------------------------------------------------------------------------------------------
void Core::resultHtml()
{
    #ifdef DEBUG
        auto start = std::chrono::high_resolution_clock::now();
        printf("%s\n","/////////////////////////////////////////////////////////////////////////");
    #endif // DEBUG
    nlohmann::json j;
    j["place"] = OffersToJson(vResult);
    j["social"] = OffersToJson(vResultSocial);
    j["clean"] = hm->place_clean;
    retHtml = j.dump();
    j.clear();
    #ifdef DEBUG
        auto elapsed = std::chrono::high_resolution_clock::now() - start;
        long long microseconds = std::chrono::duration_cast<std::chrono::microseconds>(elapsed).count();
        printf("Time %s taken: %lld \n", __func__,  microseconds);
        printf("%s\n","/////////////////////////////////////////////////////////////////////////");
    #endif // DEBUG
}
//-------------------------------------------------------------------------------------------------------------------
void Core::RISAlgorithm(const Offer::Map &items)
{
    request_processed_++;
    #ifdef DEBUG
        auto start = std::chrono::high_resolution_clock::now();
        printf("%s\n","------------------------------------------------------------------");
        printf("RIS offers %lu \n",items.size());
    #endif // DEBUG
    unsigned int loopCount;
    if( items.size() == 0)
    {
        hm->place_clean = true;
        #ifdef DEBUG
        std::clog<<"["<<tid<<"]"<<typeid(this).name()<<"::"<<__func__<< " error items size: 0 "
             <<params->get_.c_str()
             <<params->post_.c_str()
            <<std::endl;
            auto elapsed = std::chrono::high_resolution_clock::now() - start;
            long long microseconds = std::chrono::duration_cast<std::chrono::microseconds>(elapsed).count();
            printf("Time %s taken return items.size() == 0: %lld \n", __func__,  microseconds);
            printf("%s\n","------------------------------------------------------------------");
        #endif // DEBUG
        return;
    }

    //sort by rating
    for(auto i = items.begin(); i != items.end(); i++)
    {
        if((*i).second)
        {
                if(!(*i).second->social)
                {
                        result.insert(Offer::PairRate((*i).second->rating, (*i).second));
                }
                else
                {
                        resultSocial.insert(Offer::PairRate((*i).second->rating, (*i).second));

                }
        }
    }
    if (result.size() < params->getCapacity())
    {
        hm->place_clean = true;
    }

    //teaser by unique id and company
    unsigned int passage;
    passage = 0;
    unsigned int unique_by_campaign;
    while ((passage < params->getCapacity()) && (vResult.size() < params->getCapacity()))
    {
        for(auto p = result.begin(); p != result.end(); ++p)
        {
            unique_by_campaign = (*p).second->unique_by_campaign + passage;
            if(OutPutCampaignSet.count((*p).second->campaign_id) < unique_by_campaign && OutPutOfferSet.count((*p).second->id_int) == 0)
            {
                if(vResult.size() >= params->getCapacity())
                {
                    break;
                }
                if((*p).second->rating > 1000.0)
                {
                    vResult.push_back((*p).second);
                    OutPutOfferSet.insert((*p).second->id_int);
                    OutPutCampaignSet.insert((*p).second->campaign_id);
                }

            }
        }
        passage++;
    }
    
    passage = 0;
    while ((passage < params->getCapacity()) && (vResultSocial.size() < params->getCapacity()))
    {
        for(auto p = resultSocial.begin(); p != resultSocial.end(); ++p)
        {
            unique_by_campaign = (*p).second->unique_by_campaign + passage;
            if(OutPutSocialCampaignSet.count((*p).second->campaign_id) < unique_by_campaign && OutPutSocialOfferSet.count((*p).second->id_int) == 0)
            {
                if(vResultSocial.size() >= params->getCapacity())
                {
                    break;
                }

                vResultSocial.push_back((*p).second);
                OutPutSocialOfferSet.insert((*p).second->id_int);
                OutPutSocialCampaignSet.insert((*p).second->campaign_id);

            }
        }
        passage++;
    }

    //teaser by unique id
    for(auto p = result.begin(); p != result.end(); ++p)
    {
        if(OutPutOfferSet.count((*p).second->id_int) == 0)
        {
            if(vResult.size() >= params->getCapacity())
            {
                break;
            }

            vResult.push_back((*p).second);
            OutPutOfferSet.insert((*p).second->id_int);
            OutPutCampaignSet.insert((*p).second->campaign_id);
        }
    }

    for(auto p = resultSocial.begin(); p != resultSocial.end(); ++p)
    {
        if(OutPutSocialOfferSet.count((*p).second->id_int) == 0)
        {
            if(vResultSocial.size() >= params->getCapacity())
            {
                break;
            }


            vResultSocial.push_back((*p).second);
            OutPutSocialOfferSet.insert((*p).second->id_int);
            OutPutSocialCampaignSet.insert((*p).second->campaign_id);
        }
    }
    loopCount = vResult.size();
    while(loopCount < params->getCapacity())
    {
        for(auto p = result.begin(); p != result.end(); ++p)
        {
            if(vResult.size() < params->getCapacity())
            {
                #ifdef DEBUG
                    printf("%s\n","place_clean in appender");
                #endif // DEBUG
                hm->place_clean = true;
                vResult.push_back((*p).second);
            }
        }
        loopCount++;
    }
    if(vResult.size() > params->getCapacity())
    {
        vResult.erase(vResult.begin() + params->getCapacity(), vResult.end());
    }
    for(auto p = vResult.begin(); p != vResult.end(); ++p)
    {
        (*p)->load();
        (*p)->gen();
        if((*p)->rating < 1000.0 )
        {
            hm->place_clean = true;
        }
        offer_processed_++;
    }
    if(vResultSocial.size() > params->getCapacity())
    {
        vResultSocial.erase(vResultSocial.begin() + params->getCapacity(), vResultSocial.end());
    }
    for(auto p = vResultSocial.begin(); p != vResultSocial.end(); ++p)
    {
        (*p)->load();
        (*p)->gen();
        social_processed_++;
    }
    #ifdef DEBUG
        auto elapsed = std::chrono::high_resolution_clock::now() - start;
        long long microseconds = std::chrono::duration_cast<std::chrono::microseconds>(elapsed).count();
        printf("Time %s taken return RIS place offers %lu: %lld \n", __func__,  vResult.size(), microseconds);
        printf("Time %s taken return RIS social offers %lu: %lld \n", __func__,  vResultSocial.size(), microseconds);
        printf("%s\n","------------------------------------------------------------------");
    #endif // DEBUG
}
