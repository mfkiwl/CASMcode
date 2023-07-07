#include "casm/casm_io/dataformatter/DataFormatter_impl.hh"
#include "casm/casm_io/dataformatter/FormattedDataFile_impl.hh"
#include "casm/database/ConfigDatabase.hh"
#include "casm/database/ScelDatabase.hh"
#include "casm/monte_carlo/MonteCarloEnum_impl.hh"
#include "casm/monte_carlo/canonical/CanonicalSettings.hh"
#include "casm/monte_carlo/grand_canonical/GrandCanonicalSettings.hh"

namespace CASM {
namespace Monte {

/// \brief Insert in hall of fame if 'check' passes
MonteCarloEnum::HallOfFameType::InsertResult MonteCarloEnum::_insert(
    const Configuration &config) {
  Configuration _config = config;
  if (insert_primitive_only()) {
    m_halloffame->insert(
        config.primitive().in_canonical_supercell().canonical_form());
  } else if (insert_canonical()) {
    m_halloffame->insert(config.in_canonical_supercell().canonical_form());
  } else {
    m_halloffame->insert(config);
  }
}

/// \brief Attempt to insert Configuration into enumeration hall of fame
///
/// \returns Pair of iterator pointing to inserted Configuration (or end), and
///          bool indicating if insert was successful
///
/// Configurations are only inserted into hall of fame if:
/// - 'enum_check' returns true
/// - the Configuration is not already in the config list
MonteCarloEnum::HallOfFameType::InsertResult MonteCarloEnum::insert(
    const Configuration &config) {
  bool check = (*m_enum_check)(config);

  if (!check) {
    if (debug()) {
      _log().custom("Config enumeration");
      _log() << "enum check: " << std::boolalpha << check << std::endl;
      _log() << std::endl;

      print_info();
    }

    return HallOfFameType::InsertResult(
        m_halloffame->end(), false, std::numeric_limits<double>::quiet_NaN(),
        false, m_halloffame->end());
  }

  auto res = _insert(config);

  if (debug()) {
    _log().custom("Config enumeration");
    _log() << "enum check: " << std::boolalpha << check << std::endl;
    _log() << "score: " << res.score << std::endl;
    _log() << "insert config in hall of fame: " << std::boolalpha << res.success
           << std::endl;
    if (!res.success) {
      if (res.excluded) {
        _log() << "already in config list: " << res.excluded_pos->second.name()
               << std::endl;
      } else if (res.pos != m_halloffame->end()) {
        _log() << "already in hall of fame: #"
               << std::distance(m_halloffame->begin(), res.pos) << std::endl;
      } else {
        _log() << "score not good enough" << std::endl;
      }
    }
    _log() << std::endl;

    print_info();
  }
  return res;
}

/// \brief const Access the enumeration hall of fame
const MonteCarloEnum::HallOfFameType &MonteCarloEnum::halloffame() const {
  if (m_halloffame) {
    return *m_halloffame;
  } else {
    throw std::runtime_error(
        "Error accessing Monte Carlo HallOfFame: was not initialized");
  }
}

/// \brief Save configurations in the hall of fame to the config list
void MonteCarloEnum::save_configs(bool dry_run) {
  if (!halloffame().size()) {
    _log().write("Enumerated configurations to master config list");
    _log() << "No configurations in hall of fame\n";
    _log() << std::endl;
    return;
  }

  std::vector<Configuration> output;
  m_data.clear();

  // transform hall of fame configurations so that they fill the canonical
  // equivalent supercell, and add to project
  for (const auto &val : halloffame()) {
    double score = val.first;
    auto insert_res = val.second.insert(save_primitive_only());

    // store config source info
    jsonParser json_src;
    std::stringstream ss;
    ss << std::setprecision(6) << score;
    json_src["monte_carlo_enumeration"]["metric"] = m_metric_args;
    json_src["monte_carlo_enumeration"]["score"] = ss.str();

    auto lambda = [&](const Configuration &config, bool is_new) {
      if (is_new && this->check_existence()) {
        // necessary if included now, but pushed out of HallOfFame later
        this->_halloffame().exclude(config);
      }

      output.push_back(config);

      // store source info
      Configuration tconfig{config};
      tconfig.push_back_source(json_src);
      auto it = this->primclex().db<Configuration>().update(tconfig);
    };

    this->m_data[val.second.name()] = std::make_tuple(
        insert_res.insert_canonical, insert_res.insert_primitive, score);

    if (insert_res.canonical_it != primclex().db<Configuration>().end()) {
      lambda(*insert_res.canonical_it, insert_res.insert_canonical);
    }

    if (insert_res.canonical_it != insert_res.primitive_it) {
      lambda(*insert_res.primitive_it, insert_res.insert_primitive);
    }
  }

  if (!dry_run) {
    primclex().db<Configuration>().commit();
    primclex().db<Supercell>().commit();
  }

  std::string args =
      "configname is_primitive is_new is_new_primitive score potential_energy "
      "comp";
  if (m_order_parameter != nullptr) {
    args += " order_parameter";
  }

  auto formatter = m_dict.parse(args);
  auto flag = FormatFlag(_log()).print_header(true);

  _log().write("Enumerated configurations to master config list");
  _log() << "configuration enumeration check: " << m_check_args << "\n";
  _log() << "configuration enumeration metric: " << m_metric_args << "\n";
  _log() << flag << formatter(halloffame().begin(), halloffame().end());
  _log() << std::endl;
}

void MonteCarloEnum::print_info() const {
  _log().custom("Enumerated configurations hall of fame");
  _log() << "configuration enumeration check: " << m_check_args << "\n";
  _log() << "configuration enumeration metric: " << m_metric_args << "\n";
  _log() << std::setw(16) << "position" << std::setw(16) << "score"
         << "\n";
  _log() << std::setw(16) << std::string("-", 12) << std::setw(16)
         << std::string("-", 12) << "\n";

  Index i = 0;
  for (const auto &val : halloffame()) {
    _log() << std::setw(16) << i << std::setw(16) << val.first << "\n";
    i++;
  }
  _log() << std::endl;
}

/// \brief Clear hall of fame and reset excluded
void MonteCarloEnum::reset() {
  m_halloffame->clear();
  if (check_existence()) {
    m_halloffame->clear_excluded();
    // pushes back ALL configurations in database into the exclude set
    const auto &db = primclex().db<Configuration>();
    m_halloffame->exclude(db.begin(), db.end());
  }
}

MonteCarloEnum::HallOfFameType &MonteCarloEnum::_halloffame() {
  if (m_halloffame) {
    return *m_halloffame;
  } else {
    throw std::runtime_error(
        "Error accessing Monte Carlo HallOfFame: was not initialized");
  }
}

}  // namespace Monte
}  // namespace CASM
