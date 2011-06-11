/** @file tree.test.cc
 *
 *  A "stress" test driver for trees.
 *
 *  @author Dongryeol Lee (dongryel@cc.gatech.edu)
 */

// for BOOST testing
#define BOOST_TEST_MAIN

#include <boost/test/unit_test.hpp>
#include "core/metric_kernels/lmetric.h"
#include "core/table/table.h"
#include "core/tree/gen_metric_tree.h"
#include "core/tree/gen_kdtree.h"
#include "core/math/math_lib.h"
#include <time.h>

namespace core {
namespace tree {

template<typename TableType>
class TestTree {

  private:

    bool TestTreeIterator_(
      typename TableType::TreeType *node,
      TableType &table) {

      typename TableType::TreeIterator node_it =
        table.get_node_iterator(node);
      do {
        core::table::DensePoint point;
        int point_id;
        double point_weight;
        node_it.Next(&point, &point_id, &point_weight);
        core::table::DensePoint compare_point;
        double compare_point_weight;
        table.get(point_id, &compare_point, &compare_point_weight);

        for(int i = 0; i < point.length(); i++) {
          if(point[i] != compare_point[i]) {
            return false;
          }
        }
        if(point_weight != compare_point_weight) {
          return false;
        }
      }
      while(node_it.HasNext());

      if(node->is_leaf() == false) {
        return TestTreeIterator_(node->left(), table) &&
               TestTreeIterator_(node->right(), table);
      }
      return true;
    }

    void GenerateRandomDataset_(
      int num_dimensions,
      int num_points,
      TableType *random_dataset) {

      random_dataset->Init(num_dimensions, num_points);

      for(int j = 0; j < num_points; j++) {
        core::table::DensePoint point;
        random_dataset->get(j, &point);
        for(int i = 0; i < num_dimensions; i++) {
          point[i] = core::math::Random(0.1, 1.0);
        }

        // Set the weight to the random one.
        random_dataset->weights().set(
          0, j, core::math::Random(1.0, 5.0));
      }
    }

  public:

    int StressTestMain() {
      for(int i = 0; i < 10; i++) {
        int num_dimensions = core::math::RandInt(3, 20);
        int num_points = core::math::RandInt(130000, 200001);
        if(StressTest(num_dimensions, num_points) == false) {
          printf("Failed!\n");
          exit(0);
        }
      }
      return 0;
    }

    bool StressTest(int num_dimensions, int num_points) {

      int leaf_size = core::math::RandInt(15, 25);
      std::cout << "Number of dimensions: " << num_dimensions << "\n";
      std::cout << "Number of points: " << num_points << "\n";
      std::cout << "Leaf size: " << leaf_size << "\n";

      // Push in the reference dataset name.
      std::string references_in("random.csv");

      // The weight dataset name.
      std::string weights_in("weights.csv");

      // Generate the random dataset and save it.
      TableType random_table;
      GenerateRandomDataset_(
        num_dimensions, num_points, &random_table);
      random_table.Save(references_in, &weights_in);

      // Reload the table twice and build the tree on one of them.
      TableType reordered_table;
      reordered_table.Init(references_in, 0, &weights_in);
      TableType original_table;
      original_table.Init(references_in, 0, &weights_in);
      core::metric_kernels::LMetric<2> l2_metric;
      reordered_table.IndexData(l2_metric, leaf_size);
      for(int i = 0; i < reordered_table.n_entries(); i++) {
        core::table::DensePoint reordered_point;
        double reordered_weight;
        core::table::DensePoint original_point;
        double original_weight;
        reordered_table.get(i, &reordered_point, &reordered_weight);
        original_table.get(i, &original_point, &original_weight);
        for(int j = 0; j < reordered_table.n_attributes(); j++) {
          if(reordered_point[j] != original_point[j]) {
            printf("Reordered points and original points do not match!\n");
            return false;
          }
        }
        if(reordered_weight != original_weight) {
          printf("Reordered point weight and the original one do not match!\n");
          return false;
        }
      }

      // Take the root bounding primitive, and generate points within
      // it and test whether it actually contains the randomly
      // generated points.
      const int num_random_points_within_bound = 1000;
      for(int k = 0; k < num_random_points_within_bound; k++) {
        core::table::DensePoint random_point;
        reordered_table.get_tree()->bound().RandomPointInside(&random_point);
        if(! reordered_table.get_tree()->bound().Contains(
              l2_metric, random_point)) {
          printf("Random point is not within the bound!\n");
          return false;
        }
      }

      // Now test the node iterator at each level of the tree.
      if(TestTreeIterator_(
            reordered_table.get_tree(), reordered_table) == false) {
        printf("Tree iterator is broken!\n");
        return false;
      }
      return true;
    }
};
}
}

BOOST_AUTO_TEST_SUITE(TestSuiteTree)
BOOST_AUTO_TEST_CASE(TestCaseTree) {

  // Table type: hard-coded for a metric tree.
  typedef core::table::Table <
  core::tree::GenMetricTree<core::tree::AbstractStatistic> >
  GenMetricTreeTableType;

  // Another table type coded for a kd-tree.
  typedef core::table::Table <
  core::tree::GenKdTree<core::tree::AbstractStatistic> > GenKdTreeTableType;

  // Call the tests.
  printf("Starting the generic metric tree test...\n");
  core::tree::TestTree<GenMetricTreeTableType> gen_metric_tree_test;
  gen_metric_tree_test.StressTestMain();
  printf("Starting the generic kd tree test...\n");
  core::tree::TestTree<GenKdTreeTableType> gen_kd_tree_test;
  gen_kd_tree_test.StressTestMain();

  std::cout << "All tests passed!\n";
}
BOOST_AUTO_TEST_SUITE_END()
