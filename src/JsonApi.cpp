#include "json.hpp"

#include "CalcTables.h"
#include "dds.h"

using namespace std;
using json = nlohmann::json;

static json TableResultToJson(const ddTableResults * table);

char * STDCALL JsonApi_CalcAllTables(const char * params)
{
  auto jobj = json::parse(params);

  ddTableDealsPBN pbnDeals;
  pbnDeals.noOfTables = 1;
  strcpy(pbnDeals.deals[0].cards, jobj["pbn"].get<string>().c_str());

  ddTablesRes table;
  allParResults pres;

  int mode = 0; // No par calculation
  int trumpFilter[DDS_STRAINS] = {0, 0, 0, 0, 0}; // All
  int res = CalcAllTablesPBN(&pbnDeals, 0, trumpFilter, &table, &pres);

  json ans;

  if (res != RETURN_NO_FAULT)
  {
    char line[80];
    ErrorMessage(res, line);    
    ans["error"] = line;
    return strdup(ans.dump().c_str());
  }

  ans = TableResultToJson(&table.results[0]);
  return strdup(ans.dump().c_str());
}

void STDCALL JsonApi_FreeCPtr(void * ptr)
{
  free(ptr);
}

static json TableResultToJson(const ddTableResults * table)
{
    json jdir;

    jdir["s"] = table->resTable[0][0];
    jdir["h"] = table->resTable[1][0];
    jdir["d"] = table->resTable[2][0];
    jdir["c"] = table->resTable[3][0];
    jdir["n"] = table->resTable[4][0];

    json ans;
    ans["north"] = jdir;

    jdir["s"] = table->resTable[0][1];
    jdir["h"] = table->resTable[1][1];
    jdir["d"] = table->resTable[2][1];
    jdir["c"] = table->resTable[3][1];
    jdir["n"] = table->resTable[4][1];

    ans["east"] = jdir;

    jdir["s"] = table->resTable[0][2];
    jdir["h"] = table->resTable[1][2];
    jdir["d"] = table->resTable[2][2];
    jdir["c"] = table->resTable[3][2];
    jdir["n"] = table->resTable[4][2];

    ans["south"] = jdir;

    jdir["s"] = table->resTable[0][3];
    jdir["h"] = table->resTable[1][3];
    jdir["d"] = table->resTable[2][3];
    jdir["c"] = table->resTable[3][3];
    jdir["n"] = table->resTable[4][3];

    ans["west"] = jdir;

    return ans;
}