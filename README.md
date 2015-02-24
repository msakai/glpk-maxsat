glpk-maxsat
===========

[![Build Status](https://secure.travis-ci.org/msakai/glpk-maxsat.png?branch=master)](http://travis-ci.org/msakai/glpk-maxsat)

Max-SAT frontend for GLPK (GNU Linear Programming Kit)
<http://www.gnu.org/software/glpk/>.

It convert problems of MAX-SAT families into MIP and solve them using
LP-based branch-and-bound approach.

Build
-----

    ./waf configure
    ./waf build

Usage
-----

    glpsol-maxsat [file.cnf|file.wcnf]

About GLPK
----------

The GLPK (GNU Linear Programming Kit) package is intended for solving
large-scale linear programming (LP), mixed integer programming (MIP),
and other related problems. It is a set of routines written in ANSI C
and organized in the form of a callable library.
