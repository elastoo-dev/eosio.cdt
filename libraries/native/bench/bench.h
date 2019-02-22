/**
 *  @file
 *  @copyright defined in eosio.cdt/LICENSE.txt
 */
#pragma once

#include <chrono>
#include <functional>
#include <limits>
#include <map>
#include <string>
#include <vector>

#include <boost/preprocessor/cat.hpp>
#include <boost/preprocessor/stringize.hpp>

namespace benchmark {
// In case high_resolution_clock is steady, prefer that, otherwise use steady_clock.
   struct best_clock {
      using hi_res_clock = std::chrono::high_resolution_clock;
      using steady_clock = std::chrono::steady_clock;
      using type = std::conditional<hi_res_clock::is_steady, hi_res_clock, steady_clock>::type;
   };
   using clock = best_clock::type;
   using time_point = clock::time_point;
   using duration = clock::duration;

   class Printer;

   class State
   {
   public:
      std::string m_name;
      uint64_t m_num_iters_left;
      const uint64_t m_num_iters;
      const uint64_t m_num_evals;
      std::vector<double> m_elapsed_results;
      time_point m_start_time;

      bool UpdateTimer(time_point finish_time);

      State(std::string name, uint64_t num_evals, double num_iters, Printer& printer) : m_name(name), m_num_iters_left(0), m_num_iters(num_iters), m_num_evals(num_evals)
      {
      }

      inline bool KeepRunning()
      {
         if (m_num_iters_left--) {
            return true;
         }

         bool result = UpdateTimer(clock::now());
         // measure again so runtime of UpdateTimer is not included
         m_start_time = clock::now();
         return result;
      }
   };

   typedef std::function<void(State&)> BenchFunction;

   class BenchRunner
   {
      struct Bench {
         BenchFunction func;
         uint64_t num_iters_for_one_second;
      };
      typedef std::map<std::string, Bench> BenchmarkMap;
      static BenchmarkMap& benchmarks();

   public:
      BenchRunner(std::string name, BenchFunction func, uint64_t num_iters_for_one_second);

      static void RunAll(Printer& printer, uint64_t num_evals, double scaling, const std::string& filter, bool is_list_only);
   };

   // interface to output benchmark results.
   class Printer
   {
   public:
      virtual ~Printer() {}
      virtual void header() = 0;
      virtual void result(const State& state) = 0;
      virtual void footer() = 0;
   };

   // default printer to console, shows min, max, median.
   class ConsolePrinter : public Printer
   {
   public:
      void header() override;
      void result(const State& state) override;
      void footer() override;
   };

   // creates box plot with plotly.js
   class PlotlyPrinter : public Printer
   {
   public:
      PlotlyPrinter(std::string plotly_url, int64_t width, int64_t height);
      void header() override;
      void result(const State& state) override;
      void footer() override;

   private:
      std::string m_plotly_url;
      int64_t m_width;
      int64_t m_height;
   };
}


// BENCHMARK(foo, num_iters_for_one_second) expands to:  benchmark::BenchRunner bench_11foo("foo", num_iterations);
// Choose a num_iters_for_one_second that takes roughly 1 second. The goal is that all benchmarks should take approximately
// the same time, and scaling factor can be used that the total time is appropriate for your system.
#define BENCHMARK(n, num_iters_for_one_second)                          \
   benchmark::BenchRunner BOOST_PP_CAT(bench_, BOOST_PP_CAT(__LINE__, n))(BOOST_PP_STRINGIZE(n), n, (num_iters_for_one_second));

