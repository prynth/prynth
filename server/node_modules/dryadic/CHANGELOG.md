0.3.0 / 2016-10-24
==================

  * support node 6
  * childContext: do not deprecate it yet. Needs more thought
  * Always return a bluebird Promise so that methods like .timeout are available.
  * add DryadPlayer.dump() - utility function to dump the polygraph and state to the log.
  * make CommandNode throw an error if callOrder flag isn't recognized.
    This saves having to import SELF_THEN_CHILDREN constant in other libraries.
    You can just use the string and it will throw an error if you typo it.
  * fix: DryadTree getDebugState didn't report 'prepared'
  * change: Rather than put updateContext and run into dryadic's own layer,
    always add them to DryadPlayer.
    They are needed for core functionality, so you need to create a DryadPlayer
    without having to use some helper function to add in the default layer.
  * Refactor to have CommandNode call down the command tree.
    API change: prepareForAdd should now return a command object just like the other
    commands (add/remove)
    Command objects can also supply a callOrder flag to specify in what order
    the CommandNode and it's children CommandNodes should be called.
    eg. This allows waiting for self to resolve before calling the children.
    This was especially necessary for Properties where the children need to complete
    prepareForAdd before executing the PropertiesOwner prepareForAdd.
  * Remove unused Dryad.tag
  * Remove ugly context.subgraph store
    This was used by Synth to get access to the dryadic properties.
    No longer needed.
  * Improve debugging for crashes and errors.
  * Simplify how the values of "dryadicProperties" are resolved.
    Prepares each sibling in series so values from properties are ready for
    the owner dryad to access.
    Currently this forces all children to prepare in series which is not ideal.
    They are much faster prepared in parallel.
  * add CommandNode class, replacing the previous plain object
  * mapProperties: better differentiation between Dryad and plain objects
  * Call accessor functions in properties when executing commands,
    pass finalized properties into each middleware.
    So a Dryad should not access it's own properties but should rather use
    those passed into the command function.
  * If a Dryad has Dryads in it's .properties then wrap those in a Properties
    parent object and replace the .properties with accessor functions.
  * add flowtype
  * add updateContext middleware
    For returning commands that will update the dryad's context.
    This make events and dynamic methods clearer to test. Rather than
    calling functions to update context inside the handler, it returns a command
    object.
  * DryadTree-updateContext: comments and tests
    The context is always updated in place, so it is misleading to reassign it to
    the context store (and doesn't do anything).
  * Fix https://github.com/crucialfelix/dryadic/issues/6 - hyperscript convert dryadic forms inside of properties
    If a properties dict contains a value that looks like a hyperscript form
    ['name', {key: value}, [ children ]] then convert that into a Dryad.
    Also adds type and error checking and throws informative errors.
  * completely remove the commented out Bluebird global promise unhandledRejection handler
  * DryadPlayer: do not globally interfere with Bluebird's promise configuration.
    That needs to be opt in.
  * reject with Error not string (for Bluebird)
  * fix(DryadPlayer): log errors but do not swallow the Promise chain
    It's nice that the error isn't invisible (and frustrating) but you need
    to catch it at the end of the line where you are using it.
    
0.2.1 / 2016-08-18
==================

  * tests collect coverage
  * documentation, minor internal change to updateContext
  * DryadPlayer: post stack trace in error handler
  * travis on latest 4 5 6

0.2.0 / 2016-04-27
==================

  * move Store out of dryadic and into supercollider.js
  * catch and log errors during .play
    hyperscript: explain expected array type error in the parse error message
  * Add initialContext(), deprecate childContext()
  * add rootContext option to supply external loggers or configuration
    that can be accessed by all Dryads in the tree.
    set context.log to console by default
    All logging in Dryads should be done using context.log
  * feat: DryadPlayer getDebugState - returns current state of each object in tree
  * tooling, ignore files for publishing
  * CHANGE: middleware functions now take a command and context rather than having
    to process the whole stack.
    ADD: command operations now update context to log success/failure for debugging

0.1.0 / 2016-04-04
==================

  * pass in player to command methods; add player.callCommand and player.updateContext
  * Add context.callCommand to allow out of band execution of command objects
    This is for spawning synths, responding to incoming data etc. without
    resorting to directly calling functions that communicate with the outside world.
    The functions still are not pure though.
  * documentation
  * rename internal method; fix all calls to _.assign to be non mutating
  * improve error checking and reporting in a few places
  * Accept hyperscript trees to dryadic() and DryadPlayer
  * add hyperscript - Convert a JSON object tree into a tree of instantiated Dryads
  * initial check in
