/**
* test very basic sharding
*/

s = new ShardingTest( "shard2" , 2 , 5);

db = s.getDB( "test" );

s.adminCommand( { partition : "test" } );
s.adminCommand( { shard : "test.foo" , key : { num : 1 } } );
assert.eq( 1 , s.config.sharding.count()  , "sanity check 1" );
assert.eq( 1, s.config.sharding.findOne().shards.length );

s.adminCommand( { split : "test.foo" , find : { num : 0 } } );
assert.eq( 1 , s.config.sharding.count() );
shard = s.config.sharding.findOne();
assert.eq( 2 , shard.shards.length );
assert.eq( shard.shards[0].server , shard.shards[1].server , "server should be the same after a split" );


db.foo.save( { num : 1 , name : "eliot" } );
db.foo.save( { num : 2 , name : "sara" } );
db.foo.save( { num : -1 , name : "joe" } );

sleep( 100 ); // TODO: remove
assert.eq( 3 , s.getServer( "test" ).getDB( "test" ).foo.find().length() );

primary = s.getServer( "test" ).getDB( "test" );
seconday = s.getOther( primary ).getDB( "test" );

assert.eq( 3 , primary.foo.find().length() );
assert.eq( 0 , seconday.foo.find().length() );

// at this point we have 2 shard on 1 server

assert.throws( function(){ s.adminCommand( { moveshard : "test.foo" , find : { num : 1 } , to : primary.getMongo().name } ); } );
assert.throws( function(){ s.adminCommand( { moveshard : "test.foo" , find : { num : 1 } , to : "adasd" } ) } );

s.adminCommand( { moveshard : "test.foo" , find : { num : 1 } , to : seconday.getMongo().name } );
assert.eq( 3 , primary.foo.find().length() );
assert.eq( 3 , seconday.foo.find().length() );


// TODO: add wrong data to primary and make sure doesn't appear in collated results

s.stop();