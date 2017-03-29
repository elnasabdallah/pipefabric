#define CATCH_CONFIG_MAIN  // This tells Catch to provide a main() - only do this in one cpp file

#include "catch.hpp"

#include <sstream>
#include <thread>
#include <chrono>
#include <future>

#include <boost/core/ignore_unused.hpp>

#include <boost/filesystem.hpp>

#include "TestDataGenerator.hpp"

#include "core/Tuple.hpp"

#include "table/Table.hpp"

#include "dsl/Topology.hpp"
#include "dsl/Pipe.hpp"
#include "dsl/PFabricContext.hpp"

using namespace pfabric;
using namespace ns_types;

TEST_CASE("Building and running a simple topology", "[Topology]") {
  typedef TuplePtr<int, std::string, double> T1;
  typedef TuplePtr<double, int> T2;

  TestDataGenerator tgen("file.csv");
  tgen.writeData(5);

  std::stringstream strm;
  std::string expected = "0.5,0\n200.5,2\n400.5,4\n";

  Topology t;
  auto s1 = t.newStreamFromFile("file.csv")
    .extract<T1>(',')
    .where([](auto tp, bool outdated) { return get<0>(tp) % 2 == 0; } )
    .map<T2>([](auto tp, bool outdated) -> T2 {
      return makeTuplePtr(get<2>(tp), get<0>(tp));
    })
    .assignTimestamps([](auto tp) { return get<1>(tp); })
    .print(strm);

  t.start();
  t.wait();
  REQUIRE(strm.str() == expected);
}

TEST_CASE("Building and running a topology with ZMQ", "[Topology]") {
  typedef TuplePtr<int, int> T1;

  zmq::context_t context (1);
  zmq::socket_t publisher (context, ZMQ_PUB);
  publisher.bind("tcp://*:5678");

  std::stringstream strm;

  Topology t;
  auto s = t.newAsciiStreamFromZMQ("tcp://localhost:5678")
    .extract<T1>(',')
    .print(strm);

  t.start(false);

  using namespace std::chrono_literals;
  std::this_thread::sleep_for(1s);

  auto handle = std::async(std::launch::async, [&publisher](){
    std::vector<std::string> input = {
      "0,10", "1,11", "2,12", "3,13", "4,14", "5,15"
    };
    for(const std::string &s : input) {
      zmq::message_t request (4);
      memcpy (request.data (), s.c_str(), 4);
      publisher.send (request);
    }
  });

  handle.get();
  std::this_thread::sleep_for(2s);

  std::string expected = "0,10\n1,11\n2,12\n3,13\n4,14\n5,15\n";

  REQUIRE(strm.str() == expected);

}

TEST_CASE("Building and running a topology with ToTable", "[Topology]") {
  typedef TuplePtr<int, std::string, double> T1;

  auto testTable = std::make_shared<Table<T1::element_type, int>>("TopTable");

  TestDataGenerator tgen("file.csv");
  tgen.writeData(10);

  Topology t;
  auto s = t.newStreamFromFile("file.csv")
    .extract<T1>(',')
    .keyBy<int>([](auto tp) { return get<0>(tp); })
    .toTable<int>(testTable);

  t.start(false);

  REQUIRE(testTable->size() == 10);

  for (int i = 0; i < 10; i++) {
    auto tp = testTable->getByKey(i);
    REQUIRE(get<0>(tp) == i);
    REQUIRE(get<1>(tp) == "This is a string field");
    REQUIRE(get<2>(tp) == i * 100 + 0.5);
  }
	testTable->drop();
}

TEST_CASE("Building and running a topology with partitioning", "[Topology]") {
  typedef TuplePtr<int, std::string, double> T1;
  typedef TuplePtr<int> T2;

  TestDataGenerator tgen("file.csv");
  tgen.writeData(1000);

  std::vector<int> results;
  std::mutex r_mutex;

  Topology t;
  auto s = t.newStreamFromFile("file.csv")
    .extract<T1>(',')
    .partitionBy([](auto tp) { return get<0>(tp) % 5; }, 5)
    .where([](auto tp, bool outdated) { return get<0>(tp) % 2 == 0; } )
    .map<T2>([](auto tp, bool outdated) -> T2 { return makeTuplePtr(get<0>(tp)); } )
    .merge()
    .notify([&](auto tp, bool outdated) {
      std::lock_guard<std::mutex> lock(r_mutex);
      int v = get<0>(tp);
      results.push_back(v);
    });

  t.start();

  using namespace std::chrono_literals;
  std::this_thread::sleep_for(2s);

  REQUIRE(results.size() == 500);

  std::sort(results.begin(), results.end());
  for (auto i = 0u; i < results.size(); i++) {
    REQUIRE(results[i] == i * 2);
  }
}

TEST_CASE("Building and running a topology with batcher", "[Topology]") {
  typedef TuplePtr<int, std::string, double> T1;
  typedef TuplePtr<int> T2;
  typedef BatchPtr<T2> B2;

  TestDataGenerator tgen("file.csv");
  tgen.writeData(1000);

  std::vector<int> results;
  std::mutex r_mutex;

  Topology t;
  auto s = t.newStreamFromFile("file.csv")
    .extract<T1>(',')
    .map<T2>([](auto tp, bool outdated) -> T2 { return makeTuplePtr(get<0>(tp)); } )
    .batch(100)
    .notify([&](auto tp, bool outdated) {
      std::lock_guard<std::mutex> lock(r_mutex);
      auto vec = get<0>(tp);
      REQUIRE(vec.size() == 100);
      for (auto& v : vec) {
        results.push_back(get<0>(v.first));
      }
    });
  t.start();

  using namespace std::chrono_literals;
  std::this_thread::sleep_for(2s);

  REQUIRE(results.size() == 1000);
  for (std::size_t i = 0; i < results.size(); i++) {
    REQUIRE(results[i] == i);
  }
}

TEST_CASE("Building and running a topology with stream generator", "[StreamGenerator]") {
    typedef TuplePtr<int,int,int> MyTuplePtr;

    auto testTable = std::make_shared<Table<MyTuplePtr::element_type, int>>("StreamGenTable");

    StreamGenerator<MyTuplePtr>::Generator streamGen ([](unsigned long n) -> MyTuplePtr {
        return makeTuplePtr((int)n, (int)n + 10, (int)n + 100); });
    unsigned long num = 1000;

    Topology t;
    auto s = t.streamFromGenerator<MyTuplePtr>(streamGen, num)
            .keyBy<int>([](auto tp) { return get<0>(tp); })
            .toTable<int>(testTable);

    t.start(false);

    REQUIRE(testTable->size() == num);

    for (unsigned int i = 0; i < num; i++) {
      auto tp = testTable->getByKey(i);
      REQUIRE(get<0>(tp) == i);
      REQUIRE(get<1>(tp) == i+10);
      REQUIRE(get<2>(tp) == i+100);
    }

    testTable->drop();
}

TEST_CASE("Building and running a topology with a memory source", "[MemorySource]") {
  typedef TuplePtr<int, std::string, double> T1;
  std::vector<T1> results;

  TestDataGenerator tgen("file.csv");
  tgen.writeData(10);

  Topology t;
  auto s = t.newStreamFromMemory<T1>("file.csv")
    .notify([&](auto tp, bool outdated) {
        results.push_back(tp);
    });

  t.prepare();
  t.start(false);

  REQUIRE(results.size() == 10);
}

TEST_CASE("Building and running a topology with grouping", "[GroupBy]") {
    typedef TuplePtr<int, double> T1;
    typedef TuplePtr<double> T2;
    typedef Aggregator1<T1, AggrSum<double>, 1> AggrStateSum;

    StreamGenerator<T1>::Generator streamGen ([](unsigned long n) -> T1 {
		if (n<5) return makeTuplePtr(0, (double)n + 0.5);
	    else return makeTuplePtr((int)n, (double)n + 0.5); });
    unsigned long num = 10;

	std::stringstream strm;
	std::string expected = "0.5\n2\n4.5\n8\n12.5\n5.5\n6.5\n7.5\n8.5\n9.5\n";

    Topology t;
    auto s = t.streamFromGenerator<T1>(streamGen, num)
        .keyBy<int>([](auto tp) { return get<0>(tp); })
        .groupBy<AggrStateSum, int>()
	.print(strm);
	t.start(false);

	REQUIRE(strm.str() == expected);
}

struct MySumState {
  MySumState() : sum(0) {}
  double sum;
};

TEST_CASE("Building and running a topology with stateful map", "[StatefulMap]") {
  typedef TuplePtr<unsigned long, double> MyTuplePtr;
  typedef TuplePtr<double> AggregationResultPtr;
  typedef StatefulMap<AggregationResultPtr, AggregationResultPtr, MySumState> TestMap;

  StreamGenerator<MyTuplePtr>::Generator streamGen ([](unsigned long n) -> MyTuplePtr {
    return makeTuplePtr(n, (double)n + 0.5);
  });
  unsigned long num = 1000;
  unsigned long tuplesProcessed = 0;

  std::vector<double> results;

  auto mapFun = [&]( const MyTuplePtr& tp, bool, TestMap::StateRepPtr state) -> AggregationResultPtr {
                    state->sum += get<1>(tp);
                    return makeTuplePtr(state->sum);
                };

  Topology t;
  auto s = t.streamFromGenerator<MyTuplePtr>(streamGen, num)
    .keyBy<0>()
    .statefulMap<AggregationResultPtr, MySumState>(mapFun)
    .notify([&](auto tp, bool outdated) {
        if (tuplesProcessed < num)
          results.push_back(get<0>(tp));
        tuplesProcessed++;
    });

  t.start(false);

  REQUIRE(results.size() == num);
  for (auto i=0u; i<num; i++) {
    if (i==0) REQUIRE(results[i] == 0.5);
    else REQUIRE(results[i] == results[i-1]+i+0.5);
  }
}

TEST_CASE("Combining tuples from two streams to one stream", "[ToStream]") {
  typedef TuplePtr<int, std::string, double> T1;

  TestDataGenerator tgen("file.csv");
  tgen.writeData(100);

  int results = 0;
  PFabricContext ctx;
  Dataflow::BaseOpPtr stream = ctx.createStream<T1>("stream");

  Topology t;
  auto s1 = t.newStreamFromFile("file.csv")
    .extract<T1>(',')
	.toStream(stream);

  auto s2 = t.newStreamFromFile("file.csv")
    .extract<T1>(',')
	.toStream(stream);

  auto s3 = t.fromStream<T1>(stream)
    .notify([&](auto tp, bool outdated) {
      results++;
    });

  t.start();
  t.wait();

  using namespace std::chrono_literals;
  std::this_thread::sleep_for(2s);

  REQUIRE(results == 200);
}

TEST_CASE("Tuplifying a stream of RDF strings", "[Tuplifier]") {
  typedef TuplePtr<std::string, std::string, std::string> Triple;
  typedef TuplePtr<std::string, std::string, std::string, std::string> RDFTuple;
  std::vector<RDFTuple> results;
  std::mutex r_mutex;

  Topology t;
  auto s = t.newStreamFromFile(std::string(TEST_DATA_DIRECTORY) + "tuplifier_test1.in")
    .extract<Triple>(',')
    .tuplify<RDFTuple>({ "http://data.org/name", "http://data.org/price", "http://data.org/someOther" }, 
        TuplifierParams::ORDERED)
    .notify([&](auto tp, bool outdated) {
      std::lock_guard<std::mutex> lock(r_mutex);
      results.push_back(tp);
    });
  t.start(false);
  REQUIRE(results.size() == 3);
}
