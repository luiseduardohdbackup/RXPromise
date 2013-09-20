//
//  RXPromise.h
//
//  Copyright 2013 Andreas Grosam
//
//  Licensed under the Apache License, Version 2.0 (the "License");
//  you may not use this file except in compliance with the License.
//  You may obtain a copy of the License at
//
//  http://www.apache.org/licenses/LICENSE-2.0
//
//  Unless required by applicable law or agreed to in writing, software
//  distributed under the License is distributed on an "AS IS" BASIS,
//  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//  See the License for the specific language governing permissions and
//  limitations under the License.

#import <Foundation/Foundation.h>


/*
 
 A RXPromise object represents the eventual result of an asynchronous function
 or method.
 
 
## Synopsis
 
 See also [RXPromise(Deferred)](@ref RXPromise(Deferred)).
 
@class RXPromise;

typedef id (^promise_promise_completionHandler_t)(id result);
typedef id (^promise_errorHandler_t)(NSError* error);
 
typedef RXPromise* (^then_block_t)(promise_completionHandler_t, promise_errorHandler_t);
typedef RXPromise* (^then_on_block_t)(dispatch_queue_t, promise_completionHandler_t, promise_errorHandler_t);


@interface RXPromise : NSObject

@property (nonatomic, readonly) BOOL isPending;
@property (nonatomic, readonly) BOOL isFulfilled;
@property (nonatomic, readonly) BOOL isRejected;
@property (nonatomic, readonly) BOOL isCancelled;

@property (nonatomic, readonly) then_t then;
 
 
+ (RXPromise *)promiseWithTask:(id(^)(void))asyncTask;
+ (RXPromise *)promiseWithQueue:(dispatch_queue_t)queue task:(id(^)(void))task;
 

+ (RXPromise*)all:(NSArray*)promises;
- (void) cancel;
- (void) bind:(RXPromise*) other;
- (RXPromise*) root;
- (id) get;
- (void) wait;

@end

@interface RXPromise(Deferred)

- (id)init;
- (void) fulfillWithValue:(id)result;
- (void) rejectWithReason:(id)error;
- (void) cancelWithReason:(id)reason;
- (void) setProgress:(id)progress;

@end
 

 
 
## Concurrency:
 
 RXPromises are itself thread safe and will not dead lock. It's safe to send them 
 messages from any thread/queue and at any time.
 
 The handlers use an "execution context" which they are executed on. The execution
 context is either explicit or implicit.
 
 If the \p then propertie's block will be used to define the success and error
 handler, the handlers will implictily run on an _unspecified_ and _concurrent_
 execution context. That is, once the promise is resolved the corresponding 
 handler MAY execute on any thread and concurrently to any other handler.
 
 If the \p thenOn propertie's block will be used to define success and error handler,
 the execution context will be explicitly defined through the first parameter 
 _queue_ of the block, which is either a serial or concurrent _dispatch queue_.
 Once the promise is resolved, the handler is guaranteed to execute on the specified
 execution context. The execution context MAY be serial or concurrent.

 Without any other synchronization means, concurrent access to shared resources 
 from within handlers is only guaranteed to be safe when they execute on the same
 and _serial_ execution context.
 
 The "execution context" for a dispatch queue is the target queue of that queue.
 The target queue is responsible for processing the success or error block.
 
 
 It's safe to specify the same execution context where the \p then or \p thenOn 
 property will be executed.
 
 
 
## Usage:
 

### An example for continuation:
 
    [self fetchUsersWithURL:url]
    .then(^id(id usersJSON){
        return [foo parseJSON:usersJSON];
     }, nil)
    .then(^id(id users){
        return [self mergeIntoMOC:users];
    }, nil)
    .thenOn(dispatch_get_main_queue(), nil,
    ^id(NSError* error) {
        // on main thread
        [self alertError:error];
        return nil;
    });
 
 
### Simultaneous Invokations:
   
 
 Perform authentication for a user, if that succeeded, simultaneously load profile
 and messages for that user, parse the JSON and create models.
 
    RXPromise* if_auth = [self.user authenticate];

    if_auth
    .then(^id(id result){ return [self.user loadProfile]; }, nil)
    .then(^id(id result){ return [self parseJSON:result]; }, nil)
    .then(^id(id result){ return [self createProfileModel:result]; }, nil)
    });

    if_auth
    .then(^id(id result){ return [self.user loadMessages]; }, nil)
    .then(^id(id result){ return [self parseJSON:result]; }, nil)
    .then(^id(id result){ return [self createMessagesModel:result]; }, nil)
    });
 
 
*/




// forward
@class RXPromise;

/*!
 @brief Type definition for the completion handler block.
 
 @discussion The completion handler will be invoked when the associated promise has
 been fulfilled.
 
 The block's return value will resolve the "returned promise" - that is, the promise
 returned from the "then" block - if there is any. The return value can be a promise 
 or any other object and \c nil. Returning anything else than an \c NSError object 
 signals success to the "returned promise", where returning a \c NSErro object will
 signal a failure the the "returned promise".
 
 @note The execution context is either the execution context specified
 when the handlers have been registered via property \p thenOn or it is
 unspecified when registred via \p then.
 
 @param result The result value set by the "asynchronous result provider" when it
 succeeded.
 
 @return An object or \c nil which resolves the "returned promise".
 
 */
typedef id (^promise_completionHandler_t)(id result);

/*!
 @brief Type definition for the error handler block.
 
 @discussion The error handler will be invoked when the associated promise has been
 rejected or cancelled. 
 
 @par The block's return value will resolve the "returned promise" - that is, the promise
 returned from the "then" block - if there is any. The return value can be a promise 
 or any other object and \c nil. In most cases the error handler will return a 
 \c NSError object in order to forward the error. However, a handler may also consider 
 to signal success in particular cases, where it returns something else than an 
 \c NSError object.
 
 @note The execution context is either the execution context specified
 when the handlers have been registered via property \p thenOn or it is
 unspecified when registred via \p then.
 
 @param error The value set by the "asynchronous result provider" when it failed.
 
 @return An object or \c nil which resolves the "returned promise".
 
 */
typedef id (^promise_errorHandler_t)(NSError* error);

/*!
 @brief Type definition of the "then block". The "then block" is the return value
 of the property \p then.
 
 @discussion  The "then block" has two parameters, the success handler block and 
 the error handler block. The handlers may be \c nil. 
 
 @par The "then block" returns a promise, the "returned promise". When the parent
 promise will be resolved the corresponding handler (if not \c nil) will be invoked
 and its return value resolves the "returned promise" (if it exists). Otherwise,
 if the handler block is \c nil the "returned promise" (if it exists) will be resolved
 with the parent promise' result value.
 
 @par The handler executes on a \e concurrent unspecified execution context.
 */
typedef RXPromise* (^then_block_t)(promise_completionHandler_t, promise_errorHandler_t) __attribute((ns_returns_retained));

/*!
 @brief Type definition of the "then_on block". The "then_on block" is the return 
 value of the property \p thenOn. 
 
 @discussion The "then_on block" has three parameters, the execution context, the
 success handler block and the error handler block. The handlers may be \c nil.
 
 @par The "then block" returns a promise, the "returned promise". When the parent 
 promise will be resolved the corresponding handler (if not \c nil) will be invoked 
 and its return value resolves the "returned promise" (if it exists). Otherwise, 
 if the handler block is \c nil the "returned promise" (if it exists) will be resolved 
 with the parent promise' result value.
 */
typedef RXPromise* (^then_on_block_t)(dispatch_queue_t,
                                      promise_completionHandler_t,
                                      promise_errorHandler_t) __attribute((ns_returns_retained));


/*!
 
 @brief A \p RXPromise object represents the eventual result of an asynchronous
 function or method.
 
 A \p RXPromise is a lightweight primitive which helps managing asynchronous patterns
 and make them easier to follow and understand. It also adds a few powerful features
 to asynchronous operations like \a continuation , \a grouping and \a cancellation
 
 
 @par \b Caution:
 
 A promise which has registered one or more handlers will not deallocate unless
 it is resolved and the handlers are executed. This implies that an asynchronous
 result provider MUST eventually resolve its promise.
 
 @par \b Concurrency:
 
 Concurrent access to shared resources is only guaranteed to be safe for accesses
 from within handlers whose promises belong to the same "promise tree".
 
 A "promise tree" is a set of promises which share the same root promise.
 
 
 @remarks Currently, it is guraranteed that concurrent access from within
 any handler from any promise to a shared resource is guaranteed to be safe.
 */

@interface RXPromise  : NSObject
    
    
/*!
 A convenient class method which returns a promise whose associated task is 
 defined with block \p task.
 
 The block is asynchronously dispatched on a private queue. The task's return 
 value will eventually resolve the returned promise.
 
 @param task The associated task to execute as a block. The return value of the 
 block will resolve the returned promise. The return value SHALL be a \c NSError
 object in order to indicate a failure. _task_ MUST NOT be \c nil.

 @par Example: @code
 self.fetchUserPromise =
 [RXPromise promiseWithBlock:^id(void) {
     id result = [self doSomethingElaborate];
     return result;
 }];
 */
+ (RXPromise *)promiseWithTask:(id(^)(void))task;
    
    
/*!
 A convenient class method which returns a promise whose associated task is 
 defined with block \p task.
 
 The block is asynchronously dispatched on the specified queue. The task's return
 value will eventually resolve the returned promise.
 
 @param queue The dispatch queue where the task will be executed.

 @param task The associated task to execute as a block. The return value of the
 block will resolve the returned promise. The return value SHALL be a \c NSError
 object in order to indicate a failure. _task_ MUST NOT be \c nil.
 
 @par Example: @code
 self.fetchUserPromise = [RXPromise promiseWithQueue:dispatch_get_main_queue() task:^id(void) {
     id result = [self doSomethingElaborate];
     return result;
 }];
 
 */
+ (RXPromise *)promiseWithQueue:(dispatch_queue_t)queue task:(id(^)(void))task;
 


/*!
 @brief Property \p then returns a block whose signature is
 @code
 RXPromise* (^)(promise_completionHandler_t onSuccess,
                promise_errorHandler_t onError) @endcode
 
 When this returned block is called it will register the completion handler and
 the error handler given as block parameters.
  
 @par The receiver will be retained and released only until after the receiver 
 has been resolved.
 
 @par The block returns a new \c RXPromise, the "returned promise", whose result will become
 the return value of either handler that gets called when the receiver will be resolved.
  
 @par When the block is invoked and the receiver is already resolved, the corresponding
 handler will be immediately asynchronously scheduled for execution on the 
 \e unspecified execution context.
 
 @par Parameter \p onSuccess and \p onError may be \c nil.
 
 @par The receiver can register zero or more handler (pairs) through clientes calling
 the block multiple times.
 
 @return Returns a block of type \p then_block_t.
 */
@property (nonatomic, readonly) then_block_t then;


/*!
 @brief Property \p thenOn returns a block whose signature is
 @code
 RXPromise* (^)(dispatch_queue_t queue, 
                promise_completionHandler_t onSuccess, 
                promise_errorHandler_t onError)
 @endcode
 
 When the block is called it will register the completion handler \p onSuccess and
 the error handler \p onError. When the receiver will be fulfilled the success handler
 will be executed on the specified queue \p queue. When the receiver will be rejected
 the error handler will be called on the specified queue \p queue.
 
 @par The receiver will be retained and released only until after the receiver has
 been resolved.
 
 @par The block returns a new \c RXPromise, the "returned promise", whose result will become
 the return value of either handler that gets called when the receiver will be resolved.
 
 @par When the block is invoked and the receiver is already resolved, the corresponding
 handler will be immediately asynchronously scheduled for execution on the
 \e specified execution context.

 @par Parameter \p onSuccess and \p onError may be \c nil.
 
 @par Parameter \p queue may be \c nil which effectivle is the same as when using the
 \c then_block_t block returned from property \p then.
 
 @par The receiver can register zero or more handler (pairs) through clientes calling
 the block multiple times.
 
 @return Returns a block of type \c then_on_block_t.
 */
@property (nonatomic, readonly) then_on_block_t thenOn;

/*!
 Returns \c YES if the receiveer is pending.
 */
@property (nonatomic, readonly) BOOL isPending;

/*!
 Returns \c YES if the receiver is fulfilled.
 */
@property (nonatomic, readonly) BOOL isFulfilled;

/*!
 Returns \c YES if the receiver is rejected.
 */
@property (nonatomic, readonly) BOOL isRejected;

/*!
 Returns \c YES if the receiver is cancelled.
 */
@property (nonatomic, readonly) BOOL isCancelled;


/*!
 Returns the parent promise - the promise which created
 the receiver.
 */
@property (nonatomic, readonly) RXPromise* parent;


/*!
 Returns the root promise.
 */
@property (nonatomic, readonly) RXPromise* root;

/*!
 Cancels the promise unless it is already resolved and then forwards the
 message to all children.
 */
- (void) cancel;

/*!
 @brief Cancels the promise with the specfied reason unless it is already resolved and
 then forwards the message wto all children.
 
 @param reason The reason. If reason is not a \c NSError object, the receiver will
 create a \c NSError object whose demain is \@"RXPromise", the error code is -1000
 and the user dictionary contains an entry with key \c NSLocalizedFailureReason whose
 value becomes parameter reason.
 */
- (void) cancelWithReason:(id)reason;


/*!
 Creates a resolver which rejects the receiver after the specified timeout value
 whose reason is a  \c NSError object with domain \@"RXPromise" and code = -1001,
 unless it has been resolved before elsewhere.
 
 @param timeout The timeout in seconds.
 
 @return Returns the receiver.
 */
- (RXPromise*) setTimeout:(NSTimeInterval)timeout;


/*!
 @brief Binds the receiver to the given promise  @p other.
 
 @discussion The receiver will take in the state of the given promise @p other, and
 vice versa: The receiver will be fulfilled or rejected according its bound promise. If the
 receiver receives a \c cancel message, the bound promise will be sent a \c cancelWithReason:
 message with the receiver's reason.<br>
 
 @attention A promise should not be bound to more than one other promise.<p>
 
 
 @par Example: @code
 - (RXPromise*) doSomethingAsync {
    self.promise = [RXPromise new];
    return self.promise;
 }
 
 - (void) handleEvent:(id)event {
    RXPromise* other = [self handleEvenAsync:event];
    [self.promise bind:other];
 }
 @endcode
 
 @param other The promise that will bind to the receiver.
 */
- (void) bind:(RXPromise*) other;


/*!
 @brief Blocks the current thread until after the receiver has been resolved, and 
 previously queued handlers have been finished.
 
 @note The method should be used for debugging and testing only.
 
 */
- (void) wait;


/*!
 @brief Runs the current run loop until after the receiver has been resolved,
 and previously queued handlers have been finished.
 
 Prerequisite: The current thread MUST have a run loop with at least one event
 source. Otherwise, the behavior is undefined.
 
 @note The method should be used for debugging and testing only.
 */
- (void) runLoopWait;


/*!
 Synchronously returns the value of the promise.
 
 Will block the current thread until after the promise has been resolved.
 
 @note The method should be used for debugging and testing only.
 
 @return Returns the _value_ of the receiver.
 */
- (id) get;


/*!
 @brief Synchronously returns the value of the promise.
 
 The current thread will be blocked until after the promise has been resolved or the
 timeout has been expired. The method does not change the state of the receiver.

 @note The method should be used for debugging and testing only.
 
 @param timeout The timeout in seconds.
 
 @return If the timeout has not been expired, returns the value of the receiver.
 Otherwise, returns an \c NSError object whose domain equals \@"RXPromise" and whose
 code equals -1001.
 
 */
- (id) getWithTimeout:(NSTimeInterval)timeout;


/*!
 @brief Method \a all returns a \p RXPromise object which will be resolved when all 
 promises in the array @p promises are fulfilled or when any of it will be rejected.
 
 @discussion The returned promise' success handler(s) (if any) will be called when 
 all given promises have been resolved successfully. The parameter @p result of the 
 success handler will be an array containing the eventual result of each promise 
 from the given array @p promises in the corresponding order.
 
 @par The returned promise' error handler(s) (if any) will be called when any given
 promise has been rejected with the reason of the failed promise.
 
 @par If any promise in the array has been rejected, all others will be send a
 @p cancelWithReason: message whose parameter @p reason is the error reason of the
 failing promise.
 
 @par \b Caution:
 The handler's return value MUST NOT be \c nil. This is due the restriction of
 \c NSArrays which cannot contain \c nil values.
 
 @par \b Example: @code
 [RXPromise all:@[self asyncA], [self asyncB]]
 .then(^id(id results){
     id result = [self asyncCWithParamA:results[0]
                                 paramB:results[1]]
     assert(result != nil);
     return result;
 },nil);
 @endcode

 @param promises A @c NSArray containing promises.

 @warning The promise is rejected with reason \c \@"parameter error" if
 the parameter @p promises is \c nil or empty.
 
 @return A new promise. 
 
 */
+ (RXPromise*)all:(NSArray*)promises;



/*!
 @brief Method \p any returns a \c RXPromise object which will be resolved when
 any promise in the array \p promises is fulfilled or when all have been rejected.
 
 @discussion
 If any of the promises in the array will be fulfilled, all others will be send a
 \c cancel message.
 
 The @p result parameter of the completion handler of the @p then property of the
 returned promise is the result of the first promise which has been fulfilled.
 
 The @p reason parameter of the error handler of the @p then property of the returned
 promise indicates that none of the promises has been fulfilled.
 
 @par \b Example:@code
 NSArray* promises = @[async_A(),
     async_B(), async_C();
 RXPromise* any = [RXPromise any:promises]
 .then(^id(id result){
     NSLog(@"first result: %@", result);
     return nil;
 },nil);
 @endcode
 
 @param promises A \c NSArray containing promises.
 
 @note The promise is rejected with reason \c \@"parameter error" if
 the parameter \p promises is \c nil or empty.
 
 @return A new promise.
 */
+ (RXPromise*)any:(NSArray*)promises;


/*!
 For each element in array \p inputs sequentially call the asynchronous task
 passing it the element as its input argument.
 

 @discussion If the task succeeds, the task will be invoked with the next input,
 if any. The eventual result of each task is ignored. If the tasks fails, no further 
 inputs will be processed and the returned promise will be resolved with the error.
 If all inputs have been processed successfully the returned promise will be 
 resoveld with @"OK".
 
 The tasks are cancelable. That is, if the returned promise will be cancelled, the
 cancel signal will be forwarded to the current running task via cancelling the
 root promise of task's returned promise.

@param inputs A array of input values.

@param task The unary task to be invoked.

@return A promise.
*/
+ (RXPromise*) sequence:(NSArray*)inputs task:(RXPromise* (^)(id input)) task;


@end



/*!
 
 See also: `RXPromise` interface.
 
 The "Deferred" interface is what the asynchronous result provider will see.
 It is responisble for creating and resolving the promise.
 
 */
@interface RXPromise(Deferred)

/*!
 @brief Returns a new promise whose state is pending.
 
 Designated Initializer
 */
- (id)init;


/*!
 @brief Fulfilles the promise with specified value.
 
 If the promise is already resolved this method has no effect.
 
 @param value The result given from the asynchronous result provider.
 */
- (void) fulfillWithValue:(id)value;


/*!
 @brief  Rejects the promise with the specified reason.
 
 If the promise is already resolved this method has no effect.
 
 @param reason The reason. If parameter \p reason is not a \c NSError object, the 
 receiver will create a \c NSError object whose demain is \@"RXPromise", the 
 error code is -1000 and the user dictionary contains an entry with key 
 \c NSLocalizedFailureReason whose value becomes parameter reason.
 */
- (void) rejectWithReason:(id)reason;


/*!
 @brief Resolves the promise with the specified result.
 
 If parameter \p result is a promise, the receiver will "bind" to the given promise, which
 includes to forward cancellation from the receiver to the given promise.
 
 @param result The result given from the asynchronous result provider which can
 can be a promise , \c nil, an \c NSError object or any other object.
 */
- (void) resolveWithResult:(id)result;



///*!
// internal
// */
//
//- (RXPromise*) registerWithQueue:(dispatch_queue_t)target_queue
//                       onSuccess:(promise_completionHandler_t)onSuccess
//                       onFailure:(promise_errorHandler_t)onFailure
//                   returnPromise:(BOOL)returnPromise    __attribute((ns_returns_retained));

@end


