#include <iostream>
#include <pqxx/pqxx>

template<typename Model_T>
void dbfetch(Model_T& obj, std::string& sql_string, bool getfn_called = false){
  try{
    pqxx::connection cxn("dbname=testdb user=root password=root host=localhost port=5432");
    pqxx::work txn(cxn);

    if(getfn_called){
      obj.records.push_back(txn.exec(sql_string).one_row());
      return;
    }

    for(const pqxx::row& row : txn.exec(sql_string)){
      obj.records.push_back(row);
    }

    txn.commit();
    return;
  }catch (const std::exception& e){
    std::cerr<<e.what()<<std::endl;
  }
}

//TODO a dbinsert fn which would likely require a populated vector of tuples by the user passed in as the object
//     and we insert tuples into the db thru this...idk if it is possible... we'll see
