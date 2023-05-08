#ifndef CASM_CanonicalSettings_impl
#define CASM_CanonicalSettings_impl

#include <boost/algorithm/string/trim.hpp>

#include "casm/app/ProjectSettings.hh"
#include "casm/app/QueryHandler.hh"
#include "casm/casm_io/container/stream_io.hh"
#include "casm/clex/Supercell.hh"
#include "casm/monte_carlo/canonical/CanonicalSettings.hh"

namespace CASM {
namespace Monte {

/// \brief Construct MonteSamplers as specified in the MonteSettings
///
/// The requested MonteSamplers are inserted in 'result' as
///   std::pair<std::string, notstd::cloneable_ptr<MonteSampler> >
///
template <typename SamplerInsertIterator>
SamplerInsertIterator CanonicalSettings::samplers(
    const PrimClex &primclex, SamplerInsertIterator result) const {
  size_type data_maxlength = max_data_length();
  std::string prop_name;
  std::string print_name;
  bool must_converge;
  double prec;
  MonteSampler *ptr;

  // copy so we can add required measurements
  std::string level1 = "data";
  std::string level2 = "measurements";
  jsonParser t_measurements = (*this)[level1][level2];

  // find existing measurements
  std::set<std::string> input_measurements;
  for (auto it = t_measurements.begin(); it != t_measurements.end(); it++) {
    // check for "all_correlations" and replace with "corr"
    std::string quantity = (*it)["quantity"].get<std::string>();
    if (quantity == "all_correlations") {
      CASM::err_log() << "Warning in setting [\"data\"][\"measurements\"]: The "
                         "quantity \"all_correlations\" is deprecated in favor "
                         "of \"corr\". Replacing with \"corr\"...";
      (*it)["quantity"] = "corr";
      quantity = "corr";
    }
    input_measurements.insert(quantity);
  }

  std::vector<std::string> required = {"potential_energy", "formation_energy"};
  if (this->make_order_parameter(primclex) != nullptr) {
    required.push_back("order_parameter");
  }

  // add required if not already requested
  for (auto it = required.begin(); it != required.end(); ++it) {
    if (std::find(input_measurements.begin(), input_measurements.end(), *it) ==
        input_measurements.end()) {
      jsonParser json;
      json["quantity"] = *it;
      t_measurements.push_back(json);
    }
  }

  try {
    for (auto it = t_measurements.cbegin(); it != t_measurements.cend(); it++) {
      prop_name = (*it)["quantity"].get<std::string>();

      // scalar quantities that we incrementally update
      std::vector<std::string> scalar_possible = {"formation_energy",
                                                  "potential_energy"};

      // check if property found is in list of possible scalar properties
      if (std::find(scalar_possible.cbegin(), scalar_possible.cend(),
                    prop_name) != scalar_possible.cend()) {
        std::tie(must_converge, prec) = _get_precision(it);

        // if 'must converge'
        if (must_converge) {
          ptr = new ScalarMonteSampler(prop_name, prop_name, prec, confidence(),
                                       data_maxlength);
        } else {
          ptr = new ScalarMonteSampler(prop_name, prop_name, confidence(),
                                       data_maxlength);
        }

        *result++ =
            std::make_pair(prop_name, notstd::cloneable_ptr<MonteSampler>(ptr));
        continue;
      }

      // scalar quantities that we incrementally update
      std::vector<std::string> vector_possible = {"non_zero_eci_correlations",
                                                  "order_parameter"};

      // check if property found is in list of possible vector properties
      if (std::find(vector_possible.cbegin(), vector_possible.cend(),
                    prop_name) != vector_possible.cend()) {
        // construct MonteSamplers for 'non_zero_eci_correlations'
        if (prop_name == "non_zero_eci_correlations") {
          result =
              _make_non_zero_eci_correlations_samplers(primclex, it, result);
        }
        // construct MonteSamplers for 'order_parameter'
        if (prop_name == "order_parameter") {
          result = _make_order_parameter_samplers(primclex, it, result);
        }
        continue;
      }

      // custom query
      _make_query_samplers(primclex, it, result);
    }

  } catch (std::runtime_error &e) {
    Log &err_log = CASM::err_log();
    err_log.error<Log::standard>(
        "'MonteSettings::samplers(const PrimClex &primclex, "
        "SamplerInsertIterator result)'");
    err_log << "Error reading [\"" << level1 << "\"][\"" << level2 << "\"]\n"
            << std::endl;
    throw;
  }

  return result;
}

template <typename jsonParserIteratorType>
std::tuple<bool, double> CanonicalSettings::_get_precision(
    jsonParserIteratorType it) const {
  if (it->contains("precision")) {
    return std::make_tuple(true, (*it)["precision"].template get<double>());
  } else {
    return std::make_tuple(false, 0.0);
  }
}

template <typename jsonParserIteratorType, typename SamplerInsertIterator>
SamplerInsertIterator
CanonicalSettings::_make_non_zero_eci_correlations_samplers(
    const PrimClex &primclex, jsonParserIteratorType it,
    SamplerInsertIterator result) const {
  size_type data_maxlength = max_data_length();
  std::string prop_name;
  std::string print_name;
  bool must_converge;
  double prec;

  MonteSampler *ptr;

  ECIContainer _eci = primclex.eci(formation_energy(primclex));

  for (size_type ii = 0; ii < _eci.index().size(); ii++) {
    prop_name = "corr";

    // store non-zero eci index in i
    size_type i = _eci.index()[ii];

    print_name = std::string("corr(") + std::to_string(i) + ")";

    std::tie(must_converge, prec) = _get_precision(it);

    // if 'must converge'
    if (must_converge) {
      ptr = new VectorMonteSampler(prop_name, i, print_name, prec, confidence(),
                                   data_maxlength);
    } else {
      ptr = new VectorMonteSampler(prop_name, i, print_name, confidence(),
                                   data_maxlength);
    }

    *result++ =
        std::make_pair(print_name, notstd::cloneable_ptr<MonteSampler>(ptr));
  }

  return result;
}

template <typename jsonParserIteratorType, typename SamplerInsertIterator>
SamplerInsertIterator CanonicalSettings::_make_order_parameter_samplers(
    const PrimClex &primclex, jsonParserIteratorType it,
    SamplerInsertIterator result) const {
  size_type data_maxlength = max_data_length();
  std::string print_name;
  bool must_converge;
  double prec;
  MonteSampler *ptr;

  int basis_size = 0;
  std::shared_ptr<OrderParameter> order_parameter =
      this->make_order_parameter(primclex);
  if (order_parameter != nullptr) {
    basis_size = order_parameter->dof_space().subspace_dim();
  }

  for (size_type i = 0; i < basis_size; i++) {
    std::string prop_name = "eta";
    print_name = std::string("order_parameter(") + std::to_string(i) + ")";

    std::tie(must_converge, prec) = _get_precision(it);

    // if 'must converge'
    if (must_converge) {
      ptr = new VectorMonteSampler(prop_name, i, print_name, prec, confidence(),
                                   data_maxlength);
    } else {
      ptr = new VectorMonteSampler(prop_name, i, print_name, confidence(),
                                   data_maxlength);
    }

    *result++ =
        std::make_pair(print_name, notstd::cloneable_ptr<MonteSampler>(ptr));
  }

  std::shared_ptr<std::vector<std::vector<int>>> order_parameter_subspaces =
      this->make_order_parameter_subspaces();
  if (order_parameter_subspaces != nullptr) {
    for (size_type i = 0; i < order_parameter_subspaces->size(); i++) {
      std::string prop_name = "eta_subspace";
      print_name =
          std::string("order_parameter_subspace(") + std::to_string(i) + ")";

      std::tie(must_converge, prec) = _get_precision(it);

      // if 'must converge'
      if (must_converge) {
        ptr = new VectorMonteSampler(prop_name, i, print_name, prec,
                                     confidence(), data_maxlength);
      } else {
        ptr = new VectorMonteSampler(prop_name, i, print_name, confidence(),
                                     data_maxlength);
      }

      *result++ =
          std::make_pair(print_name, notstd::cloneable_ptr<MonteSampler>(ptr));
    }
  }

  return result;
}

template <typename jsonParserIteratorType, typename SamplerInsertIterator>
SamplerInsertIterator CanonicalSettings::_make_query_samplers(
    const PrimClex &primclex, jsonParserIteratorType it,
    SamplerInsertIterator result) const {
  size_type data_maxlength = max_data_length();
  double must_converge;
  double prec;
  std::string prop_name = (*it)["quantity"].template get<std::string>();
  MonteSampler *ptr;

  const auto &dict = primclex.settings().query_handler<Configuration>().dict();

  typedef QueryMonteSampler::Formatter FormatterType;
  std::shared_ptr<FormatterType> formatter =
      std::make_shared<FormatterType>(dict.parse(prop_name));

  // make example config to test:
  Supercell tscel(const_cast<PrimClex *>(&primclex), simulation_cell_matrix());
  Configuration config(tscel);
  config.init_occupation();
  Eigen::VectorXd test = formatter->get().evaluate_as_matrix(config).row(0);
  auto col = formatter->get().col_header(config);

  if (test.size() != col.size()) {
    std::stringstream ss;
    ss << "Error constructing Monte Carlo samplers from query: '" << prop_name
       << "'";
    Log &err_log = CASM::err_log();
    err_log << ss.str();
    err_log << "headers: " << col << std::endl;
    err_log << "  Some queries may not be available for sampling at this time."
            << std::endl;
    throw std::runtime_error(ss.str());
  }

  for (int i = 0; i < col.size(); ++i) {
    std::string print_name = col[i];
    boost::algorithm::trim(print_name);

    std::tie(must_converge, prec) = _get_precision(it);

    // if 'must converge'
    if (must_converge) {
      ptr = new QueryMonteSampler(formatter, i, print_name, prec, confidence(),
                                  data_maxlength);
    } else {
      ptr = new QueryMonteSampler(formatter, i, print_name, confidence(),
                                  data_maxlength);
    }

    *result++ =
        std::make_pair(print_name, notstd::cloneable_ptr<MonteSampler>(ptr));
  }

  return result;
}

}  // namespace Monte
}  // namespace CASM

#endif
