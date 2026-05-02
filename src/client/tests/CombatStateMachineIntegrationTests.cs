using Godot;
using System;
using DarkAges.Combat;

namespace DarkAges.Tests
{
    /// <summary>
    /// [CLIENT_AGENT] Combat State Machine Integration Tests
    /// Validates CombatStateMachineController state transitions, cooldowns, and blocking rules
    /// </summary>
    public partial class CombatStateMachineIntegrationTests : Node
    {
        [Export] public bool RunTests = false;

        private int _testsPassed = 0;
        private int _testsFailed = 0;
        private CombatStateMachineController _fsm;

        public override void _Ready()
        {
            // Create and attach FSM for testing
            _fsm = new CombatStateMachineController();
            AddChild(_fsm);
        }

        public override void _Process(double delta)
        {
            if (RunTests && _testsPassed == 0 && _testsFailed == 0)
            {
                RunAllTests();
            }
        }

        private void RunAllTests()
        {
            GD.Print("=== CombatStateMachine Integration Tests ===\n");

            Test_IdleToAttack_TransitionSucceeds();
            Test_AttackBlocksSecondAttack();
            Test_AttackBlocksDodge();
            Test_DodgeRecoversAttackAbility();
            Test_HitInterruptsAttack();
            Test_DeathBlocksAllInputs();
            Test_MovementToCombatTransitions();
            Test_GcdExpiryRestoresAttack();

            GD.Print($"\n=== Results: {_testsPassed} passed, {_testsFailed} failed ===");
            if (_testsFailed == 0)
            {
                GD.Print("All FSM integration tests PASSED!");
            }
            else
            {
                GD.PrintErr("Some FSM tests FAILED — review output above");
            }
        }

        // TEST 1: Idle → Attack should succeed, set state, start GCD
        private void Test_IdleToAttack_TransitionSucceeds()
        {
            GD.Print("Test: Idle→Attack transition...");
            try
            {
                // Ensure starting idle
                _fsm.SetMovementState(true, false);
                _fsm.CurrentState.ShouldBe(CombatState.Idle);

                bool result = _fsm.TryAttack();
                result.ShouldBeTrue("TryAttack from idle should succeed");

                _fsm.CurrentState.ShouldBe(CombatState.Attack, "State should become Attack");
                _fsm.IsBusy.ShouldBeTrue("FSM should be busy during attack");
                _fsm.IsOnCooldown.ShouldBeTrue("Global cooldown should start");

                GD.Print("  ✅ Idle→Attack transition works, GCD started");
                _testsPassed++;
            }
            catch (Exception ex)
            {
                GD.PrintErr($"  ❌ {ex.Message}");
                _testsFailed++;
            }
        }

        // TEST 2: Attacking → second attack should be blocked
        private void Test_AttackBlocksSecondAttack()
        {
            GD.Print("Test: Attack blocks second attack...");
            try
            {
                _fsm.SetMovementState(true, false);
                _fsm.TryAttack(); // First attack
                bool secondAttempt = _fsm.TryAttack();

                secondAttempt.ShouldBeFalse("Second TryAttack while busy should fail");
                _fsm.CurrentState.ShouldBe(CombatState.Attack, "State should remain Attack");

                GD.Print("  ✅ Attack correctly blocks follow-up attack while busy");
                _testsPassed++;
            }
            catch (Exception ex)
            {
                GD.PrintErr($"  ❌ {ex.Message}");
                _testsFailed++;
            }
        }

        // TEST 3: During attack, dodge should be blocked
        private void Test_AttackBlocksDodge()
        {
            GD.Print("Test: Attack blocks dodge...");
            try
            {
                _fsm.SetMovementState(true, false);
                _fsm.TryAttack();
                bool dodgeResult = _fsm.TryDodge();

                dodgeResult.ShouldBeFalse("TryDodge during attack should fail");
                _fsm.CurrentState.ShouldBe(CombatState.Attack, "State should still be Attack");

                GD.Print("  ✅ Dodge correctly blocked while attacking");
                _testsPassed++;
            }
            catch (Exception ex)
            {
                GD.PrintErr($"  ❌ {ex.Message}");
                _testsFailed++;
            }
        }

        // TEST 4: After attack finishes, dodge should become possible
        private void Test_DodgeRecoversAttackAbility()
        {
            GD.Print("Test: Dodge recovers after attack...");
            try
            {
                _fsm.SetMovementState(true, false);
                _fsm.TryAttack();
                _fsm.CurrentState.ShouldBe(CombatState.Attack);

                // Simulate attack animation completing
                _fsm.TriggerAttackCompleted();

                _fsm.IsBusy.ShouldBeFalse("FSM should not be busy after attack completes");
                bool dodgeNow = _fsm.TryDodge();
                dodgeNow.ShouldBeTrue("Dodge should be available after attack ends");
                _fsm.CurrentState.ShouldBe(CombatState.Dodge);

                GD.Print("  ✅ Dodge becomes available after attack recovery");
                _testsPassed++;
            }
            catch (Exception ex)
            {
                GD.PrintErr($"  ❌ {ex.Message}");
                _testsFailed++;
            }
        }

        // TEST 5: Hit can interrupt attack (Hit overrides IsBusy rule)
        private void Test_HitInterruptsAttack()
        {
            GD.Print("Test: Hit interrupts attack...");
            try
            {
                _fsm.SetMovementState(true, false);
                _fsm.TryAttack();
                _fsm.CurrentState.ShouldBe(CombatState.Attack);
                _fsm.IsBusy.ShouldBeTrue();

                // Hit should forcibly interrupt
                _fsm.TriggerHit();
                _fsm.CurrentState.ShouldBe(CombatState.Hit, "State should become Hit");
                // Hit should clear busy flag to allow recovery
                _fsm.IsBusy.ShouldBeFalse("Hit should clear busy state");

                GD.Print("  ✅ Hit correctly interrupts attack state");
                _testsPassed++;
            }
            catch (Exception ex)
            {
                GD.PrintErr($"  ❌ {ex.Message}");
                _testsFailed++;
            }
        }

        // TEST 6: Death blocks all state transitions
        private void Test_DeathBlocksAllInputs()
        {
            GD.Print("Test: Death blocks all inputs...");
            try
            {
                _fsm.SetMovementState(false, false);
                _fsm.TriggerDeath();
                _fsm.CurrentState.ShouldBe(CombatState.Death);

                bool attack = _fsm.TryAttack();
                attack.ShouldBeFalse("Attack must fail in death state");

                bool dodge = _fsm.TryDodge();
                dodge.ShouldBeFalse("Dodge must fail in death state");

                _fsm.TryRespawn();
                _fsm.CurrentState.ShouldBe(CombatState.Death, "Cannot exit death without explicit state override");

                GD.Print("  ✅ Death state correctly blocks all combat inputs");
                _testsPassed++;
            }
            catch (Exception ex)
            {
                GD.PrintErr($"  ❌ {ex.Message}");
                _testsFailed++;
            }
        }

        // TEST 7: Movement state changes preserve combat availability
        private void Test_MovementToCombatTransitions()
        {
            GD.Print("Test: Movement→Combat transitions...");
            try
            {
                // Walk → Idle → Attack
                _fsm.SetMovementState(true, false); // Walk
                _fsm.CurrentState.ShouldBe(CombatState.Walk);

                _fsm.SetMovementState(true, true); // Sprint → Run
                _fsm.CurrentState.ShouldBe(CombatState.Run);

                _fsm.SetMovementState(false, false); // Stop → Idle
                _fsm.CurrentState.ShouldBe(CombatState.Idle);

                bool canAttack = _fsm.TryAttack();
                canAttack.ShouldBeTrue("Attack available from idle");
                _fsm.CurrentState.ShouldBe(CombatState.Attack);

                GD.Print("  ✅ Movement→Idle→Attack flow valid");
                _testsPassed++;
            }
            catch (Exception ex)
            {
                GD.PrintErr($"  ❌ {ex.Message}");
                _testsFailed++;
            }
        }

        // TEST 8: GCD expiry restores attack ability
        private void Test_GcdExpiryRestoresAttack()
        {
            GD.Print("Test: GCD expiry restores attack...");
            try
            {
                _fsm.SetMovementState(true, false);
                _fsm.TryAttack();
                _fsm.IsOnCooldown.ShouldBeTrue();

                // Simulate time passing to clear cooldown
                _fsm.ForceAdvanceCooldown(5.0f); // > combattime_constants::ABILITY_COOLDOWN_MS

                _fsm.IsOnCooldown.ShouldBeFalse("Cooldown should be cleared");
                bool canAttackAgain = _fsm.TryAttack();
                canAttackAgain.ShouldBeTrue("Attack available after GCD expires");

                GD.Print("  ✅ GCD expiry correctly restores attack");
                _testsPassed++;
            }
            catch (Exception ex)
            {
                GD.PrintErr($"  ❌ {ex.Message}");
                _testsFailed++;
            }
        }

        // ============================================================
        // ASSERTION HELPERS
        // ============================================================
        private static class Assert
        {
            public static void AreEqual<T>(T expected, T actual, string message = "")
            {
                if (!EqualityComparer<T>.Default.Equals(expected, actual))
                {
                    throw new Exception($"Assert failed: {message}\n  Expected: {expected}\n  Actual: {actual}");
                }
            }

            public static void IsTrue(bool condition, string message = "")
            {
                if (!condition) throw new Exception($"Assert failed: {message} (expected true, got false)");
            }

            public static void IsFalse(bool condition, string message = "")
            {
                if (condition) throw new Exception($"Assert failed: {message} (expected false, got true)");
            }
        }

        // Extension helpers for clean syntax
        private static class Extensions
        {
            public static void ShouldBe<T>(this T actual, T expected, string message = "")
            {
                Assert.AreEqual(expected, actual, message);
            }

            public static void ShouldBeTrue(this bool actual, string message = "")
            {
                Assert.IsTrue(actual, message);
            }

            public static void ShouldBeFalse(this bool actual, string message = "")
            {
                Assert.IsFalse(actual, message);
            }
        }
    }
}
