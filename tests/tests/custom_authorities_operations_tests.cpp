/*
 * Copyright (c) 2019 BitShares Core Team, and contributors.
 *
 * The MIT License
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include <boost/test/unit_test.hpp>
#include <graphene/chain/protocol/custom_authority.hpp>
#include <graphene/chain/custom_authority_object.hpp>
#include <graphene/chain/operation_type_to_id.hpp>
#include <fc/crypto/digest.hpp>

#include "../common/database_fixture.hpp"

using namespace graphene::chain;
using namespace graphene::chain::test;
using namespace graphene::app;

struct custom_authorities_operations_fixture: database_fixture
{
   custom_authorities_operations_fixture()
   {
      nathan_key = fc::ecc::private_key::generate();
      nathan = &create_account("nathan", nathan_key.get_public_key());
      core = &asset_id_type()(db);
      
      fund(*nathan);
   }
   
   void create_custom_authority(const account_id_type& account, bool enabled, int operation_type, const std::vector<restriction_v2>& restrictions = {})
   {
      custom_authority_create_operation op; 
      op.account = account;
      op.enabled = enabled;
      op.valid_from = db.head_block_time() - 1;
      op.valid_to = db.head_block_time() + 20;
      op.operation_type = operation_type;
      op.restrictions = restrictions;
      
      trx.operations.push_back(op);
      trx.validate();
      
      db.push_transaction(trx, ~0);
      trx.operations.clear();
   }
   
   void update_account( const account_object& account)
   {
      trx.operations.clear();
      
      account_update_operation op;
      op.account = account.id;
      op.active = account.active;
      
      trx.operations = {op};
      trx.validate();
      
      db.push_transaction(trx, ~0);
      trx.clear();
   }
   
   void push_transfer_operation_from_nathan_to_core()
   {
      transfer_operation op;
      op.from = nathan->id;
      op.to = account_id_type();
      op.amount = core->amount(500);
      
      trx.operations = {op};
      
      sign(trx, nathan_key);
      
      PUSH_TX( db, trx, database::skip_transaction_dupe_check );
      
      trx.operations.clear();
   }
   
   fc::ecc::private_key nathan_key;
   const account_object* nathan = nullptr;
   const asset_object* core = nullptr;
};

BOOST_FIXTURE_TEST_SUITE( custom_authorities_operations, custom_authorities_operations_fixture )

BOOST_AUTO_TEST_CASE(get_custom_authorities_by_account_without_authorities) {
   try {
      auto dan = create_account("dan");
      
      BOOST_CHECK(db.get_custom_authorities_by_account(dan.id).empty());
   } catch (fc::exception &e) {
      edump((e.to_detail_string()));
      throw;
   }
}

BOOST_AUTO_TEST_CASE(get_custom_authorities_by_account_without_authorities_but_with_authorities_for_another_account) {
   try {
      auto dan = create_account("dan");
      auto sam = create_account("sam");
      
      create_custom_authority(sam.id, true, int_from_operation_type<transfer_operation>::value);
      
      auto authorities = db.get_custom_authorities_by_account(dan.id);
      BOOST_REQUIRE_EQUAL(0, authorities.size());
   } catch (fc::exception &e) {
      edump((e.to_detail_string()));
      throw;
   }
}

BOOST_AUTO_TEST_CASE(create_custom_authority_operation_adds_authority_to_db) {
   try {
      auto dan = create_account("dan");

      custom_authority_create_operation op;
      op.account = dan.id;
      op.enabled = true;
      op.valid_from = time_point_sec(1);
      op.valid_to = time_point_sec(2);
      op.operation_type = int_from_operation_type<transfer_operation>::value;
      
      eq_restriction rest;
      rest.argument = "amount";
      rest.value = asset(100);
      
      op.restrictions = {rest};

      trx.operations.push_back(op);
      trx.validate();
      
      db.push_transaction(trx, ~0);
      trx.operations.clear();
      
      auto authorities = db.get_custom_authorities_by_account(dan.id);
      BOOST_REQUIRE_EQUAL(1, authorities.size());
      BOOST_CHECK(dan.id == authorities.front().account);
      BOOST_CHECK(authorities.front().enabled);
      BOOST_CHECK(time_point_sec(1) == authorities.front().valid_from);
      BOOST_CHECK(time_point_sec(2) == authorities.front().valid_to);
      BOOST_CHECK(int_from_operation_type<transfer_operation>::value == authorities.front().operation_type.value);

      BOOST_REQUIRE_EQUAL(1, authorities.front().restrictions.size());
      
      auto restriction = authorities.front().restrictions.front().get<eq_restriction>();
      BOOST_CHECK_EQUAL("amount", restriction.argument);
      BOOST_CHECK(asset(100) == restriction.value.get<asset>());
      
   } catch (fc::exception &e) {
      edump((e.to_detail_string()));
      throw;
   }
}

BOOST_AUTO_TEST_CASE(delete_custom_authority) {
   try {
      auto dan = create_account("dan");
      
      custom_authority_create_operation op;
      op.account = dan.id;
      op.enabled = true;
      op.valid_from = time_point_sec(1);
      op.valid_to = time_point_sec(2);
      op.operation_type = int_from_operation_type<transfer_operation>::value;
      
      eq_restriction rest;
      rest.argument = "amount";
      rest.value = asset(100);
      
      op.restrictions = {rest};
      
      trx.operations.push_back(op);
      trx.validate();
      processed_transaction ptx = db.push_transaction(trx, ~0);
      trx.operations.clear();
      
      auto authorities = db.get_custom_authorities_by_account(dan.id);
      BOOST_REQUIRE_EQUAL(1, authorities.size());
      
      {
         custom_authority_delete_operation op;
         op.custom_authority_to_delete = authorities.front().id;
         
         trx.operations.push_back(op);
         trx.validate();
         processed_transaction ptx = db.push_transaction(trx, ~0);
         trx.operations.clear();
      }
      
      authorities = db.get_custom_authorities_by_account(dan.id);
      BOOST_REQUIRE_EQUAL(0, authorities.size());
      
   } catch (fc::exception &e) {
      edump((e.to_detail_string()));
      throw;
   }
}

BOOST_AUTO_TEST_CASE(custom_authority_is_disabled_after_account_update) {
   try {
      auto dan = create_account("dan");
      
      create_custom_authority(dan.id, true, int_from_operation_type<custom_authority_create_operation>::value);
      create_custom_authority(dan.id, true, int_from_operation_type<account_update_operation>::value);
      
      update_account(dan);
      
      auto authorities = db.get_custom_authorities_by_account(dan.id);
      BOOST_REQUIRE_EQUAL(2, authorities.size());
      
      auto auth = authorities[0];
      BOOST_CHECK(dan.id == auth.account);
      BOOST_CHECK(int_from_operation_type<custom_authority_create_operation>::value == auth.operation_type.value);
      
      BOOST_REQUIRE_EQUAL(0, auth.restrictions.size());
      
      //authority should be disabled
      BOOST_CHECK(!auth.enabled);
      
      auth = authorities[1];
      BOOST_CHECK(dan.id == auth.account);
      BOOST_CHECK(int_from_operation_type<account_update_operation>::value == auth.operation_type.value);
      
      BOOST_REQUIRE_EQUAL(0, auth.restrictions.size());
      
      //authority should be disabled
      BOOST_CHECK(!auth.enabled);
      
   } catch (fc::exception &e) {
      edump((e.to_detail_string()));
      throw;
   }
}

BOOST_AUTO_TEST_CASE(custom_authority_is_disabled_after_account_update_with_serveral_accounts) {
   try {
      auto dan = create_account("dan");
      
      create_custom_authority(dan.id, true, int_from_operation_type<custom_authority_create_operation>::value);
      create_custom_authority(dan.id, true, int_from_operation_type<account_update_operation>::value);
      
      auto sam = create_account("sam");
      
      create_custom_authority(sam.id, true, int_from_operation_type<custom_authority_create_operation>::value);
      create_custom_authority(sam.id, true, int_from_operation_type<account_update_operation>::value);
      
      update_account(dan);
      
      {
         auto authorities = db.get_custom_authorities_by_account(dan.id);
         BOOST_REQUIRE_EQUAL(2, authorities.size());
         
         auto auth = authorities[0];
         BOOST_CHECK(dan.id == auth.account);
         BOOST_CHECK(int_from_operation_type<custom_authority_create_operation>::value == auth.operation_type.value);
         
         BOOST_REQUIRE_EQUAL(0, auth.restrictions.size());
         
         //authority should be disabled
         BOOST_CHECK(!auth.enabled);
         
         auth = authorities[1];
         BOOST_CHECK(dan.id == auth.account);
         BOOST_CHECK(int_from_operation_type<account_update_operation>::value == auth.operation_type.value);
         
         BOOST_REQUIRE_EQUAL(0, auth.restrictions.size());
         
         //authority should be disabled
         BOOST_CHECK(!auth.enabled);
      }
      
      {
         auto authorities = db.get_custom_authorities_by_account(sam.id);
         BOOST_REQUIRE_EQUAL(2, authorities.size());
         
         auto auth = authorities[0];
         BOOST_CHECK(sam.id == auth.account);
         BOOST_CHECK(int_from_operation_type<custom_authority_create_operation>::value == auth.operation_type.value);
         BOOST_REQUIRE_EQUAL(0, auth.restrictions.size());
         BOOST_CHECK(auth.enabled);
         
         auth = authorities[1];
         BOOST_CHECK(sam.id == auth.account);
         BOOST_CHECK(int_from_operation_type<account_update_operation>::value == auth.operation_type.value);
         BOOST_REQUIRE_EQUAL(0, auth.restrictions.size());
         BOOST_CHECK(auth.enabled);
      }
      
   } catch (fc::exception &e) {
      edump((e.to_detail_string()));
      throw;
   }
}

BOOST_AUTO_TEST_CASE(transaction_passes_without_authorities_installed) {
   try {
      
      BOOST_CHECK_NO_THROW(push_transfer_operation_from_nathan_to_core());

   } catch (fc::exception &e) {
      edump((e.to_detail_string()));
      throw;
   }
}

BOOST_AUTO_TEST_CASE(transaction_fails_with_authorities_installed) {
   try {
      custom_authority_create_operation op;
      op.account = nathan->id;
      op.enabled = true;
      op.valid_from = time_point_sec(1); //validation will fail because of not valid interval
      op.valid_to = time_point_sec(2);
      op.operation_type = int_from_operation_type<transfer_operation>::value;
      
      trx.operations.push_back(op);
      trx.validate();
      
      db.push_transaction(trx, ~0);
      trx.operations.clear();
      
      auto authorities = db.get_custom_authorities_by_account(nathan->id);
      BOOST_REQUIRE_EQUAL(1, authorities.size());
      
      BOOST_CHECK_THROW(push_transfer_operation_from_nathan_to_core(), fc::exception);
   } catch (fc::exception &e) {
      edump((e.to_detail_string()));
      throw;
   }
}

BOOST_AUTO_TEST_CASE(transaction_passes_with_authorities_installed) {
   try {
      create_custom_authority(nathan->id, true, int_from_operation_type<transfer_operation>::value);
      
      auto authorities = db.get_custom_authorities_by_account(nathan->id);
      BOOST_REQUIRE_EQUAL(1, authorities.size());
      
      BOOST_CHECK_NO_THROW(push_transfer_operation_from_nathan_to_core());
   } catch (fc::exception &e) {
      edump((e.to_detail_string()));
      throw;
   }
}

BOOST_AUTO_TEST_CASE(transaction_passes_with_one_authority_passed_and_one_failed) {
   try {
      create_custom_authority(nathan->id, true, int_from_operation_type<custom_authority_create_operation>::value);
      create_custom_authority(nathan->id, true, int_from_operation_type<transfer_operation>::value);
      
      auto authorities = db.get_custom_authorities_by_account(nathan->id);
      BOOST_REQUIRE(!authorities.empty());
      
      BOOST_CHECK_NO_THROW(push_transfer_operation_from_nathan_to_core());
   } catch (fc::exception &e) {
      edump((e.to_detail_string()));
      throw;
   }
}

BOOST_AUTO_TEST_CASE(transaction_fails_with_one_authority_failed_and_one_disabled) {
   try {
      create_custom_authority(nathan->id, true, int_from_operation_type<custom_authority_create_operation>::value);
      create_custom_authority(nathan->id, false, int_from_operation_type<transfer_operation>::value);
      
      auto authorities = db.get_custom_authorities_by_account(nathan->id);
      BOOST_REQUIRE(!authorities.empty());
      
      BOOST_CHECK_THROW(push_transfer_operation_from_nathan_to_core(), fc::exception);
   } catch (fc::exception &e) {
      edump((e.to_detail_string()));
      throw;
   }
}

BOOST_AUTO_TEST_CASE(transaction_fails_with_one_failed_restriction) {
   try {
      
      eq_restriction restriction;
      restriction.value = asset(400);
      restriction.argument = "amount";
      
      create_custom_authority(nathan->id, true, int_from_operation_type<transfer_operation>::value, {restriction});
      
      auto authorities = db.get_custom_authorities_by_account(nathan->id);
      BOOST_REQUIRE(!authorities.empty());
      
      BOOST_CHECK_THROW(push_transfer_operation_from_nathan_to_core(), fc::exception);
   } catch (fc::exception &e) {
      edump((e.to_detail_string()));
      throw;
   }
}

BOOST_AUTO_TEST_CASE(transaction_succeeds_with_one_restriction) {
   try {
      
      eq_restriction restriction;
      restriction.value = asset(500);
      restriction.argument = "amount";
      
      create_custom_authority(nathan->id, true, int_from_operation_type<transfer_operation>::value, {restriction});
      
      auto authorities = db.get_custom_authorities_by_account(nathan->id);
      BOOST_REQUIRE(!authorities.empty());
      
      BOOST_CHECK_NO_THROW(push_transfer_operation_from_nathan_to_core());
   } catch (fc::exception &e) {
      edump((e.to_detail_string()));
      throw;
   }
}

BOOST_AUTO_TEST_CASE(limit_order_succeeds_with_custom_authority)
{
   create_custom_authority(nathan->id, true, int_from_operation_type<limit_order_create_operation>::value);
   
   auto* test = &create_bitasset("test");
   
   limit_order_create_operation op;
   op.seller = nathan->id;
   op.amount_to_sell = core->amount(500);
   op.min_to_receive = test->amount(500);
   op.expiration = db.head_block_time() + fc::seconds(10);
   
   trx.operations = {op};
   
   BOOST_CHECK_NO_THROW(PUSH_TX( db, trx, ~0 ));
}

BOOST_AUTO_TEST_CASE(limit_order_fails_with_custom_authority)
{
   neq_restriction restriction;
   restriction.value = asset(500);
   restriction.argument = "amount_to_sell";
   
   create_custom_authority(nathan->id, true, int_from_operation_type<limit_order_create_operation>::value, {restriction});
   
   auto* test = &create_bitasset("test");
   
   limit_order_create_operation op;
   op.seller = nathan->id;
   op.amount_to_sell = core->amount(500);
   op.min_to_receive = test->amount(500);
   op.expiration = db.head_block_time() + fc::seconds(10);
   
   trx.operations = {op};
   
   BOOST_CHECK_THROW(PUSH_TX( db, trx, ~0 ), fc::exception);
}

BOOST_AUTO_TEST_SUITE_END()
