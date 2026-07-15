#include "common/motor_crc_hg.h"
#include "rclcpp/rclcpp.hpp"
#include "unitree_hg/msg/imu_state.hpp"
#include "unitree_hg/msg/low_cmd.hpp"
#include "unitree_hg/msg/low_state.hpp"


using namespace std::chrono_literals;

const auto HG_CMD_TOPIC = "lowcmd";
const auto HG_IMU_TORSO = "secondary_imu";
const auto HG_STATE_TOPIC = "lowstate";
constexpr float PI = 3.14159265358979323846F;
template <typename T>
class DataBuffer
{
public:
  void SetData(const T &new_data)
  {
    std::lock_guard<std::mutex> const lock(mutex_);
    data_ = std::make_shared<T>(new_data);
  }

  std::shared_ptr<const T> GetData()
  {
    std::lock_guard<std::mutex> const lock(mutex_);
    return data_ ? data_ : nullptr;
  }

  void Clear()
  {
    std::lock_guard<std::mutex> lock(mutex_);
    data_ = nullptr;
  }

private:
  std::shared_ptr<T> data_;
  std::mutex mutex_;
};

const int G1_NUM_MOTOR = 29;
struct ImuState
{
  std::array<float, 3> rpy = {};
  std::array<float, 3> omega = {};
};

struct MotorCommand
{
  std::array<float, G1_NUM_MOTOR> q_target = {};
  std::array<float, G1_NUM_MOTOR> dq_target = {};
  std::array<float, G1_NUM_MOTOR> kp = {};
  std::array<float, G1_NUM_MOTOR> kd = {};
  std::array<float, G1_NUM_MOTOR> tau_ff = {};
};

struct MotorState
{
  std::array<float, G1_NUM_MOTOR> q = {};
  std::array<float, G1_NUM_MOTOR> dq = {};
};

// Stiffness for all G1 Joints
const std::array<float, G1_NUM_MOTOR> Kp{
    60, 60, 60, 100, 40, 40,    // legs
    60, 60, 60, 100, 40, 40,    // legs
    60, 40, 40,                 // waist
    40, 40, 40, 40, 40, 40, 40, // arms
    40, 40, 40, 40, 40, 40, 40  // arms
};

// Damping for all G1 Joints
const std::array<float, G1_NUM_MOTOR> Kd{
    1, 1, 1, 2, 1, 1,    // legs
    1, 1, 1, 2, 1, 1,    // legs
    1, 1, 1,             // waist
    1, 1, 1, 1, 1, 1, 1, // arms
    1, 1, 1, 1, 1, 1, 1  // arms
};

enum class Mode
{
  PR = 0, // Series Control for Pitch/Roll Joints
  AB = 1  // Parallel Control for A/B Joints
};

enum G1JointIndex
{
  LEFT_HIP_PITCH = 0,
  LEFT_HIP_ROLL = 1,
  LEFT_HIP_YAW = 2,
  LEFT_KNEE = 3,
  LEFT_ANKLE_PITCH = 4,
  LEFT_ANKLE_B = 4,
  LEFT_ANKLE_ROLL = 5,
  LEFT_ANKLE_A = 5,
  RIGHT_HIP_PITCH = 6,
  RIGHT_HIP_ROLL = 7,
  RIGHT_HIP_YAW = 8,
  RIGHT_KNEE = 9,
  RIGHT_ANKLE_PITCH = 10,
  RIGHT_ANKLE_B = 10,
  RIGHT_ANKLE_ROLL = 11,
  RIGHT_ANKLE_A = 11,
  WAIST_YAW = 12,
  WAIST_ROLL = 13,  // NOTE INVALID for g1 23dof/29dof with waist locked
  WAIST_A = 13,     // NOTE INVALID for g1 23dof/29dof with waist locked
  WAIST_PITCH = 14, // NOTE INVALID for g1 23dof/29dof with waist locked
  WAIST_B = 14,     // NOTE INVALID for g1 23dof/29dof with waist locked
  LEFT_SHOULDER_PITCH = 15,
  LEFT_SHOULDER_ROLL = 16,
  LEFT_SHOULDER_YAW = 17,
  LEFT_ELBOW = 18,
  LEFT_WRIST_ROLL = 19,
  LEFT_WRIST_PITCH = 20, // NOTE INVALID for g1 23dof
  LEFT_WRIST_YAW = 21,   // NOTE INVALID for g1 23dof
  RIGHT_SHOULDER_PITCH = 22,
  RIGHT_SHOULDER_ROLL = 23,
  RIGHT_SHOULDER_YAW = 24,
  RIGHT_ELBOW = 25,
  RIGHT_WRIST_ROLL = 26,
  RIGHT_WRIST_PITCH = 27, // NOTE INVALID for g1 23dof
  RIGHT_WRIST_YAW = 28    // NOTE INVALID for g1 23dof
};

class CubicJointTrajectory
{
public:
  CubicJointTrajectory(double startAngle, double targetAngle, double duration):
  duration_(duration),
  a0_(startAngle),
  a1_(0.0),
  a2_(3.0 / (duration * duration) * (targetAngle - startAngle)),
  a3_(-2.0 / (duration * duration * duration) * (targetAngle - startAngle) )
  {
  }

  double angleAt(double time) {
     double t = std::clamp(time, 0.0, duration_);
     return a0_ + a1_ * t + a2_ * t * t + a3_ * t * t * t;
  }

private:
  double duration_;
  double a0_, a1_, a2_, a3_;
};

class JointTransition
{
public:
  JointTransition(G1JointIndex joint, double currentRotation, double targetRotation, double duration):
  joint_(joint),
  traj_(CubicJointTrajectory(currentRotation, targetRotation, duration))
  {
  }

  double GetRotation(double time)
  {
    return traj_.angleAt(time);
  }

private:
  G1JointIndex joint_;
  CubicJointTrajectory traj_;
};

class G1WaveSender : public rclcpp::Node
{
public:
  G1WaveSender() : Node("g1_wave_sender"), mode_machine_(6)
  {
    MotorCommand motorCommandInit;
    for (int i = 0; i < G1_NUM_MOTOR; ++i)
    {
      motorCommandInit.tau_ff.at(i) = 0.0;
      motorCommandInit.q_target.at(i) = 0.0;
      motorCommandInit.dq_target.at(i) = 0.0;
      motorCommandInit.kp.at(i) = Kp[i];
      motorCommandInit.kd.at(i) = Kd[i];
    }
    motorCmdBuffer.SetData(motorCommandInit);
    
    //  subscribe to "/lowstate" topic
    lowstate_subscriber_ = this->create_subscription<unitree_hg::msg::LowState>(
        HG_STATE_TOPIC, 10,
        [this](unitree_hg::msg::LowState::SharedPtr message)
        {
          LowStateHandler(message);
        });
    imustate_subscriber_ = this->create_subscription<unitree_hg::msg::IMUState>(
        HG_IMU_TORSO, 20,
        [this](unitree_hg::msg::IMUState::SharedPtr message)
        {
          IMUStateHandler(message);
        });
    // the mLowcmdPublisher is set to publish "/lowcmd" topic
    lowcmd_publisher_ =
        this->create_publisher<unitree_hg::msg::LowCmd>(HG_CMD_TOPIC, 10);

    // Run Control() function every 2ms (500 times a second or 500hz)
    controlTimer = this->create_wall_timer(std::chrono::milliseconds(2),
                                           [this]
                                           { Control(); });
    writeTimer = this->create_wall_timer(std::chrono::milliseconds(2),
                                         [this]
                                         { writeLowCommand(); });
  }

private:
  rclcpp::Subscription<unitree_hg::msg::LowState>::SharedPtr
      lowstate_subscriber_;
  rclcpp::Subscription<unitree_hg::msg::IMUState>::SharedPtr
      imustate_subscriber_;
  rclcpp::Publisher<unitree_hg::msg::LowCmd>::SharedPtr
      lowcmd_publisher_;

  Mode mode_pr_{Mode::PR};
  std::atomic<uint8_t> mode_machine_;

  rclcpp::TimerBase::SharedPtr controlTimer;
  rclcpp::TimerBase::SharedPtr writeTimer;

  DataBuffer<MotorCommand> motorCmdBuffer;
  DataBuffer<MotorState> motor_state_buffer_;

  DataBuffer<MotorState> preResetState;

  JointTransition *currentMovementYaw = nullptr;
  JointTransition *currentMovementPitch = nullptr;
  bool waveInitialized = false;
  uint32_t waveStepCount = 0;

  int step = 0;
  double stepStartTime = 0;

  double time = 0;

  void LowStateHandler(unitree_hg::msg::LowState::SharedPtr message)
  {
    MotorState msTmp;
    for (int i = 0; i < G1_NUM_MOTOR; ++i)
    {
      msTmp.q.at(i) = message->motor_state[i].q;
      msTmp.dq.at(i) = message->motor_state[i].dq;
      if ((message->motor_state[i].motorstate != 0U) && i <= RIGHT_ANKLE_ROLL)
      {
        RCLCPP_INFO(this->get_logger(), "[ERROR] motor %d with code %d", i,
                    message->motor_state[i].motorstate);
      }
    }
    motor_state_buffer_.SetData(msTmp);
  }

  void IMUStateHandler(unitree_hg::msg::IMUState::SharedPtr message)
  {
  }

  void Control()
  {
    const double deltatime = 0.002; // 500hz

    MotorCommand localCmdBuffer;
    const std::shared_ptr<const MotorState> currentState = motor_state_buffer_.GetData();

    // reproduce current state before writing change on top
    for (int i = 0; i < G1_NUM_MOTOR; ++i)
    {
      localCmdBuffer.tau_ff.at(i) = motorCmdBuffer.GetData()->tau_ff.at(i);
      localCmdBuffer.q_target.at(i) = motorCmdBuffer.GetData()->q_target.at(i);
      localCmdBuffer.dq_target.at(i) = motorCmdBuffer.GetData()->dq_target.at(i);
      localCmdBuffer.kp.at(i) = Kp[i];
      localCmdBuffer.kd.at(i) = Kd[i];
    }

    if (currentState)
    { // if a motor state is reported
      time += deltatime;

      double raiseArmTime = 1.69;
      double defaultPoseTime = 5.0;
      double holdTime = 0.4;
      double resetTime = 1.5;
      double waveStepTime = 0.8;

      double const final_pitch = -(89 * PI / 180.0); // conversion degree -> radian
      double const final_roll = (90 * PI / 180.0);

      switch (step)
      {
      case 0: // Default pose (all angles 0)
      {
        // - set to default pose
        for (int i = 0; i < G1_NUM_MOTOR; ++i)
        {
          double const ratio =
              std::clamp(time / defaultPoseTime, 0.0, 1.0);
          localCmdBuffer.q_target.at(i) =
              static_cast<float>(1.0 - ratio) * currentState->q.at(i);
        }

        if (time >= (stepStartTime + defaultPoseTime))
        {
          step++;
          stepStartTime = time;
        }

        break;
      }
      case 1: // Raise arm
      {
        if (preResetState.GetData() == nullptr)
        {
          preResetState.SetData(*(motor_state_buffer_.GetData()));
        }

        // - Raise Arm
        double t = time - stepStartTime;
        localCmdBuffer.q_target.at(LEFT_SHOULDER_PITCH) = final_pitch * std::sin(PI * (t / raiseArmTime) / 2); // -45 Grad im Bogenmaß nach vorne
        localCmdBuffer.kp.at(LEFT_SHOULDER_PITCH) = 40.0;
        localCmdBuffer.kd.at(LEFT_SHOULDER_PITCH) = 1.5;

        localCmdBuffer.q_target.at(LEFT_WRIST_ROLL) = final_roll * std::sin(PI * (t / raiseArmTime) / 2); // -45 Grad im Bogenmaß nach vorne
        localCmdBuffer.kp.at(LEFT_WRIST_ROLL) = 40.0;
        localCmdBuffer.kd.at(LEFT_WRIST_ROLL) = 1.5;

        if (time >= (stepStartTime + raiseArmTime))
        {
          step++;
          stepStartTime = time;
        }

        break;
      }
      case 2: // Hold position
      {
        // - Hold Arm
        localCmdBuffer.q_target.at(LEFT_SHOULDER_PITCH) = final_pitch;
        localCmdBuffer.kp.at(LEFT_SHOULDER_PITCH) = 40.0;
        localCmdBuffer.kd.at(LEFT_SHOULDER_PITCH) = 1.5;

        localCmdBuffer.q_target.at(LEFT_WRIST_ROLL) = final_roll;
        localCmdBuffer.kp.at(LEFT_WRIST_ROLL) = 40.0;
        localCmdBuffer.kd.at(LEFT_WRIST_ROLL) = 1.5;

        if (time >= (stepStartTime + holdTime))
        {
          step++;
          stepStartTime = time;
        }

        break;
      }

      case 3: // Wave left
      case 5:
      {
        const double leftWaveRotation = 30 * PI / 180.0;
        // left wave
        if (currentMovementYaw == nullptr)
        {
          currentMovementYaw = new JointTransition(LEFT_SHOULDER_YAW, currentState->q.at(LEFT_SHOULDER_YAW), leftWaveRotation, waveStepTime);
        }

        localCmdBuffer.q_target.at(LEFT_SHOULDER_YAW) = currentMovementYaw->GetRotation(time - stepStartTime);
        localCmdBuffer.kp.at(LEFT_SHOULDER_YAW) = 40.0;
        localCmdBuffer.kd.at(LEFT_SHOULDER_YAW) = 1.5;

        if (time >= (stepStartTime + waveStepTime))
        {
          step++;
          stepStartTime = time;
          delete currentMovementYaw;
          currentMovementYaw = nullptr;
        }

        break;
      }

      case 4: // Wave right
      case 6:
      {

        const double rightWaveRotation = -30 * PI / 180.0;
        if (currentMovementYaw == nullptr)
        {
          currentMovementYaw = new JointTransition(LEFT_SHOULDER_YAW, currentState->q.at(LEFT_SHOULDER_YAW), rightWaveRotation, waveStepTime);
        }
        // right wave
        localCmdBuffer.q_target.at(LEFT_SHOULDER_YAW) = currentMovementYaw->GetRotation(time - stepStartTime);
        localCmdBuffer.kp.at(LEFT_SHOULDER_YAW) = 40.0;
        localCmdBuffer.kd.at(LEFT_SHOULDER_YAW) = 1.5;

        if (time >= (stepStartTime + waveStepTime))
        {
          step++;
          stepStartTime = time;
          delete currentMovementYaw;
          currentMovementYaw = nullptr;
        }

        break;
      }

      case 7: // Reset position
      {
        // - Reset Arm
        double t = time - stepStartTime;
        if (currentMovementYaw == nullptr)
        {
          currentMovementYaw = new JointTransition(LEFT_SHOULDER_YAW, currentState->q.at(LEFT_SHOULDER_YAW), 0.0, resetTime);
          currentMovementPitch = new JointTransition(LEFT_SHOULDER_PITCH, currentState->q.at(LEFT_SHOULDER_PITCH), 0.0, resetTime);
        }

        const double ratio =
            std::clamp((time - stepStartTime) / resetTime, 0.0, 1.0);
        for (int i = 0; i < G1_NUM_MOTOR; ++i)
        {
          localCmdBuffer.q_target.at(i) =
              static_cast<float>(1.0 - ratio) * preResetState.GetData()->q.at(i);
        }
        localCmdBuffer.q_target.at(LEFT_SHOULDER_YAW) = currentMovementYaw->GetRotation(t);
        localCmdBuffer.q_target.at(LEFT_SHOULDER_PITCH) = currentMovementPitch->GetRotation(t);

        localCmdBuffer.q_target.at(LEFT_WRIST_ROLL) = final_roll * (1 - std::sin(PI * (t / resetTime) / 2));
        localCmdBuffer.kp.at(LEFT_WRIST_ROLL) = 40.0;
        localCmdBuffer.kd.at(LEFT_WRIST_ROLL) = 1.5;

        if (time >= (stepStartTime + resetTime))
        {
          step++;
          stepStartTime = time;

          delete currentMovementYaw;
          currentMovementYaw = nullptr;

          delete currentMovementPitch;
          currentMovementPitch = nullptr;
        }
        break;
      }
      default:
        // done
        break;
      }
    }
    motorCmdBuffer.SetData(localCmdBuffer);
  }

  void writeLowCommand()
  {
    unitree_hg::msg::LowCmd low_command;
    low_command.mode_pr = static_cast<uint8_t>(mode_pr_);
    low_command.mode_machine = mode_machine_;

    const std::shared_ptr<const MotorCommand> mc =
        motorCmdBuffer.GetData();
    if (mc)
    {
      for (size_t i = 0; i < G1_NUM_MOTOR; i++)
      {
        low_command.motor_cmd.at(i).mode = 1; // 1:Enable, 0:Disable
        low_command.motor_cmd.at(i).tau = mc->tau_ff.at(i);
        low_command.motor_cmd.at(i).q = mc->q_target.at(i);
        low_command.motor_cmd.at(i).dq = mc->dq_target.at(i);
        low_command.motor_cmd.at(i).kp = mc->kp.at(i);
        low_command.motor_cmd.at(i).kd = mc->kd.at(i);
      }

      get_crc(low_command);
      lowcmd_publisher_->publish(low_command);
    }
  }
};

int main(int argc, char **argv)
{
  rclcpp::init(argc, argv);

  auto node = std::make_shared<G1WaveSender>();

  rclcpp::spin(node);
  rclcpp::shutdown();

  return 0;
}