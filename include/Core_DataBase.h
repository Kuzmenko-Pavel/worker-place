#ifndef CORE_DATABASE_H
#define CORE_DATABASE_H

#include <string>
#include <set>

#include "Informer.h"
#include "Params.h"
#include "Offer.h"
#include "TempTable.h"
#include "DataBase.h"

class Core_DataBase
{
    public:
        Informer *informer;
        Core_DataBase();
        virtual ~Core_DataBase();

        bool getOffers(Offer::Map &items,Params *params, bool get_social);
        bool getInformer(const std::string informer_id);
        void getCampaign(Params *params);
        bool getRetargeting(Offer::Map &items,Params *params);
        bool getRetargetingAccount(Offer::Map &items,Params *params);
        bool clearTmp();

    protected:
    private:
        Params *params;
        char *cmd;
        size_t len;
        TempTable *tmpTable;
};

#endif // CORE_DATABASE_H
