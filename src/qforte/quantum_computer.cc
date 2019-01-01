#include <map>

#include "fmt/format.h"

#include "quantum_gate.h"
#include "quantum_computer.h"

std::string Basis::str(size_t nqubit) const {
    std::string s;
    s += "|";
    for (int i = 0; i < nqubit; ++i) {
        if (get_bit(i)) {
            s += "1";
        } else {
            s += "0";
        }
    }
    s += ">";
    return s;
}

Basis& Basis::insert(size_t pos) {
    basis_t temp(state_);
    state_ = state_ << 1;
    basis_t mask = (1 << pos) - 1;
    state_ = state_ ^ ((state_ ^ temp) & mask);
    return *this;
}

std::vector<std::string> QuantumCircuit::str() const {
    std::vector<std::string> s;
    for (const auto& gate : gates_) {
        s.push_back(gate.str());
    }
    return s;
}

QuantumComputer::QuantumComputer(int nqubit) : nqubit_(nqubit) {
    nbasis_ = std::pow(2, nqubit_);
    basis_.assign(nbasis_, Basis());
    coeff_.assign(nbasis_, 0.0);
    new_coeff_.assign(nbasis_, 0.0);
    for (size_t i = 0; i < nbasis_; i++) {
        basis_[i] = Basis(i);
    }
    coeff_[0] = 1.;
}

std::complex<double> QuantumComputer::coeff(const Basis& basis) { return coeff_[basis.add()]; }

void QuantumComputer::set_state(std::vector<std::pair<Basis, double_c>> state) {
    std::fill(coeff_.begin(), coeff_.end(), 0.0);
    for (const auto& basis_c : state) {
        coeff_[basis_c.first.add()] = basis_c.second;
    }
}

void QuantumComputer::apply_circuit(const QuantumCircuit& qc) {
    for (const auto& gate : qc.gates()) {
        apply_gate(gate);
    }
}

void QuantumComputer::apply_gate(const QuantumGate& qg) {
    int nqubits = qg.nqubits();

    if (nqubits == 1) {
        apply_1qubit_gate(qg);
    }

    coeff_ = new_coeff_;
    std::fill(new_coeff_.begin(), new_coeff_.end(), 0.0);
}

void QuantumComputer::apply_1qubit_gate(const QuantumGate& qg) {
    size_t target = qg.target();
    const auto& gate = qg.gate();

    for (size_t i = 0; i < 2; i++) {
        for (size_t j = 0; j < 2; j++) {
            auto op_i_j = gate[i][j];
            for (const Basis& basis_J : basis_) {
                if (basis_J.get_bit(target) == j) {
                    Basis basis_I = basis_J;
                    basis_I.set_bit(target, i);
                    new_coeff_[basis_I.add()] += op_i_j * coeff_[basis_J.add()];
                }
            }
        }
    }
}

void QuantumComputer::apply_1qubit_gate_insertion(const QuantumGate& qg) {
    size_t target = qg.target();
    const auto& gate = qg.gate();

    Basis basis_I, basis_J;
    size_t nbasis_minus1 = std::pow(2, nqubit_ - 1);
    for (size_t i = 0; i < 2; i++) {
        for (size_t j = 0; j < 2; j++) {
            auto op_i_j = gate[i][j];
            for (size_t K = 0; K < nbasis_minus1; K++) {
                Basis basis_K(K);
                basis_I = basis_J = basis_K.insert(target);
                basis_I.set_bit(target, i);
                basis_J.set_bit(target, j);
                new_coeff_[basis_I.add()] += op_i_j * coeff_[basis_J.add()];
            }
        }
    }
}

void QuantumComputer::apply_2qubit_gate(const QuantumGate& qg) {
    const auto& two_qubits_basis = QuantumGate::two_qubits_basis();

    size_t target = qg.target();
    size_t control = qg.control();
    const auto& gate = qg.gate();

    for (size_t i = 0; i < 4; i++) {
        const auto i_c = two_qubits_basis[i].first;
        const auto i_t = two_qubits_basis[i].second;
        for (size_t j = 0; j < 4; j++) {
            const auto j_c = two_qubits_basis[j].first;
            const auto j_t = two_qubits_basis[j].second;
            auto op_i_j = gate[i][j];
            for (const Basis& basis_J : basis_) {
                if ((basis_J.get_bit(control) == j_c) and (basis_J.get_bit(target) == j_t)) {
                    Basis basis_I = basis_J;
                    basis_I.set_bit(control, i_c);
                    basis_I.set_bit(target, i_t);
                    new_coeff_[basis_I.add()] += op_i_j * coeff_[basis_J.add()];
                }
            }
        }
    }
}

std::vector<std::string> QuantumComputer::str() const {
    std::vector<std::string> terms;
    for (size_t i = 0; i < nbasis_; i++) {
        if (std::abs(coeff_[i]) >= print_threshold_) {
            terms.push_back(fmt::format("({:f} {:+f} i) {}", std::real(coeff_[i]),
                                        std::imag(coeff_[i]), basis_[i].str(nqubit_)));
        }
    }
    return terms;
}
