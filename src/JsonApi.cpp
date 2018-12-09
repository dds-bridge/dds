#include <rapidjson/document.h>
#include <rapidjson/writer.h>

#include "CalcTables.h"
#include "dds.h"

using namespace std;
using namespace rapidjson;
// using json = nlohmann::json;

static char * TableResultToJson(const ddTableResults * table);
static char * ErrorToJson(const char * error);

char * STDCALL JsonApi_CalcAllTables(const char * params)
{
  Document jobj;
  jobj.Parse(params);

  ddTableDealsPBN pbnDeals;
  pbnDeals.noOfTables = 1;
  strcpy(pbnDeals.deals[0].cards, jobj["pbn"].GetString());

  ddTablesRes table;
  allParResults pres;

  int mode = 0; // No par calculation
  int trumpFilter[DDS_STRAINS] = {0, 0, 0, 0, 0}; // All
  int res = CalcAllTablesPBN(&pbnDeals, 0, trumpFilter, &table, &pres);

  if (res != RETURN_NO_FAULT)
  {
    char line[80];
    ErrorMessage(res, line);
    return ErrorToJson(line);
  }

  return TableResultToJson(&table.results[0]);
}

void STDCALL JsonApi_FreeCPtr(void * ptr)
{
  free(ptr);
}

static char * ErrorToJson(const char * error)
{
  StringBuffer sb;
  Writer<StringBuffer> writer(sb);

  writer.StartObject();

  writer.Key("error");
  writer.String(error);

  writer.EndObject();

  return strdup(sb.GetString());
}

static char * TableResultToJson(const ddTableResults * table)
{
  StringBuffer sb;
  Writer<StringBuffer> writer(sb);

  writer.StartObject();

  writer.Key("north");
  writer.StartObject();
  writer.Key("s");
  writer.Int(table->resTable[0][0]);
  writer.Key("h");
  writer.Int(table->resTable[1][0]);
  writer.Key("d");
  writer.Int(table->resTable[2][0]);
  writer.Key("c");
  writer.Int(table->resTable[3][0]);
  writer.Key("n");
  writer.Int(table->resTable[4][0]);
  writer.EndObject();

  writer.Key("east");
  writer.StartObject();
  writer.Key("s");
  writer.Int(table->resTable[0][1]);
  writer.Key("h");
  writer.Int(table->resTable[1][1]);
  writer.Key("d");
  writer.Int(table->resTable[2][1]);
  writer.Key("c");
  writer.Int(table->resTable[3][1]);
  writer.Key("n");
  writer.Int(table->resTable[4][1]);
  writer.EndObject();

  writer.Key("south");
  writer.StartObject();
  writer.Key("s");
  writer.Int(table->resTable[0][2]);
  writer.Key("h");
  writer.Int(table->resTable[1][2]);
  writer.Key("d");
  writer.Int(table->resTable[2][2]);
  writer.Key("c");
  writer.Int(table->resTable[3][2]);
  writer.Key("n");
  writer.Int(table->resTable[4][2]);
  writer.EndObject();

  writer.Key("west");
  writer.StartObject();
  writer.Key("s");
  writer.Int(table->resTable[0][3]);
  writer.Key("h");
  writer.Int(table->resTable[1][3]);
  writer.Key("d");
  writer.Int(table->resTable[2][3]);
  writer.Key("c");
  writer.Int(table->resTable[3][3]);
  writer.Key("n");
  writer.Int(table->resTable[4][3]);
  writer.EndObject();

  writer.EndObject();

  return strdup(sb.GetString());
}