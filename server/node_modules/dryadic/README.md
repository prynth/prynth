dryadic
=======

[![npm version](https://badge.fury.io/js/dryadic.svg)](http://badge.fury.io/js/dryadic) [![Build Status](https://travis-ci.org/crucialfelix/dryadic.svg?branch=master)](https://travis-ci.org/crucialfelix/dryadic) [![Dependency Status](https://david-dm.org/crucialfelix/dryadic.svg)](https://david-dm.org/crucialfelix/dryadic) [![devDependency Status](https://david-dm.org/crucialfelix/dryadic/dev-status.svg)](https://david-dm.org/crucialfelix/dryadic#info=devDependencies)

ALPHA, work in progress

>> A dryad (/ˈdraɪ.æd/; Greek: Δρυάδες, sing.: Δρυάς) is a tree nymph, or female tree spirit, in Greek mythology

A Dryad is a component for managing the creation and control of something.

That 'something' could be a SuperCollider Synth, or a MIDI connection, an SVG node or Canvas in a webrowser, a datasource, a web resource to fetch or an external process.

It is anything that you want to specify parameters for and then create according to those parameters.

Each component manages the creation, configuration, updating and removal of that something.

It is similar to React in that it is declarative and (coming soon) you can diff a tree of Dryads with a new tree and each component can execute just the changes needed to transition to the new desired tree.

https://doc.esdoc.org/github.com/crucialfelix/dryadic/


Compatibility
-------------

Works on Node 4+

Contribute
----------

- Issue Tracker: https://github.com/crucialfelix/dryadic/issues
- Source Code: https://github.com/crucialfelix/dryadic

License
-------

The project is licensed under the MIT license.
