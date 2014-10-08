// Reaktor is a C++ library for computational reaction modelling.
//
// Copyright (C) 2014 Allan Leal
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program. If not, see <http://www.gnu.org/licenses/>.

#include "OptimumUtils.hpp"

// Reaktor includes
#include <Reaktor/Common/Macros.hpp>

namespace Reaktor {

OptimumProblem::OptimumProblem(unsigned n, unsigned m)
: n(n), m(m), l(arma::zeros(n)), u(INFINITY*arma::ones(n))
{}

auto OptimumProblem::setObjective(const ObjectiveFunction& objective) -> void
{
    f = objective;
}

auto OptimumProblem::setConstraint(const ConstraintFunction& constraint) -> void
{
    h = constraint;
}

auto OptimumProblem::setLowerBounds(const Vector& lower) -> void
{
    Assert(lower.size() == n, "Dimension of the upper bound vector does not match dimension of primal variables.");
    l = lower;
}

auto OptimumProblem::setUpperBounds(const Vector& upper) -> void
{
    Assert(upper.size() == n, "Dimension of the upper bound vector does not match dimension of primal variables.");
    u = upper;
}

auto OptimumProblem::numVariables() const -> unsigned
{
    return n;
}

auto OptimumProblem::numConstraints() const -> unsigned
{
    return m;
}

auto OptimumProblem::objective() const -> const ObjectiveFunction&
{
    return f;
}

auto OptimumProblem::constraint() const -> const ConstraintFunction&
{
    return h;
}

auto OptimumProblem::lowerBounds() const -> const Vector&
{
    return l;
}

auto OptimumProblem::upperBounds() const -> const Vector&
{
    return u;
}

} // namespace Reaktor
