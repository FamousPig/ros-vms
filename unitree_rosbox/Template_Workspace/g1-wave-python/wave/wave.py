import rclpy
from rclpy.node import Node

import unitree_hg.msg._low_cmd as lowcmd
import unitree_hg.msg._low_state as lowstate
import unitree_hg.msg._motor_cmd as motorcmd



from threading import Lock

from math import pi
import array

G1_NUM_MOTOR=29

class G1JointIndex():
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
  WAIST_ROLL = 13,  # INVALID for g1 23dof/29dof with waist locked
  WAIST_A = 13,     # INVALID for g1 23dof/29dof with waist locked
  WAIST_PITCH = 14, # INVALID for g1 23dof/29dof with waist locked
  WAIST_B = 14,     # INVALID for g1 23dof/29dof with waist locked
  LEFT_SHOULDER_PITCH = 15,
  LEFT_SHOULDER_ROLL = 16,
  LEFT_SHOULDER_YAW = 17,
  LEFT_ELBOW = 18,
  LEFT_WRIST_ROLL = 19,
  LEFT_WRIST_PITCH = 20, # INVALID for g1 23dof
  LEFT_WRIST_YAW = 21,   # INVALID for g1 23dof
  RIGHT_SHOULDER_PITCH = 22,
  RIGHT_SHOULDER_ROLL = 23,
  RIGHT_SHOULDER_YAW = 24,
  RIGHT_ELBOW = 25,
  RIGHT_WRIST_ROLL = 26,
  RIGHT_WRIST_PITCH = 27, # INVALID for g1 23dof
  RIGHT_WRIST_YAW = 28    # INVALID for g1 23dof

KP = array.array('f', (
60, 60, 60, 100, 40, 40,    # legs
60, 60, 60, 100, 40, 40,    # legs
60, 40, 40,                 # waist
40, 40, 40, 40, 40, 40, 40, # arms
40, 40, 40, 40, 40, 40, 40  # arms
))

KD = array.array('f', (
    1, 1, 1, 2, 1, 1,    # legs
    1, 1, 1, 2, 1, 1,    # legs
    1, 1, 1,             # waist
    1, 1, 1, 1, 1, 1, 1, # arms
    1, 1, 1, 1, 1, 1, 1  # arms
))

class MotorCommand():
    q_target = array.array('f', (0,)*29)
    dq_target = array.array('f', (0,)*29)
    kp = KP
    kd = KD
    tau_ff = array.array('f', (0,)*29)

class MotorState():
    q = array.array('f', (0,)*29)
    dq = array.array('f', (0,)*29)

class DataBuffer():
    
    def __init__(self):
        self.lock = Lock()
        self.__data = None

    def getData(self):
        self.lock.acquire()
        returnValue = self.__data
        self.lock.release()
        return returnValue
    
    def setData(self, newData):
        self.lock.acquire()
        self.__data = newData
        self.lock.release()

    def clear(self):
        self.lock.acquire()
        self.__data = None
        self.lock.release()

class CubicJointTrajectory():
    def __init__(self, startAngle: float, targetAngle: float, duration: float):
        self.__duration = duration
        self.__a0 = startAngle
        self.__a1 = 0.0
        self.__a2 = 3.0 / duration**2 * (targetAngle - startAngle)
        self.__a3 = -2.0 / duration**3 * (targetAngle - startAngle)

    def angleAt(self, time: float):
        t = time if time <= self.__duration else self.__duration
        return self.__a0 + self.__a1 * t + self.__a2 * t**2 + self.__a3 * t**3


class WaveNode(Node):

    def __init__(self):
        super().__init__('wave_node')
        timer_period = 0.002
        self.control_timer = self.create_timer(timer_period, self.control)
        # self.write_timer = self.create_timer(timer_period, self.writeLowCmd)
        self.low_state_subscriber = self.create_subscription(lowstate.LowState, 'lowstate', self.lowStateHandler, 10)
        self.joint = G1JointIndex.LEFT_SHOULDER_PITCH;
        self.deltatime = timer_period
        self.control_time = 0.0

        self.motorState = DataBuffer()
        self.motorCommand = DataBuffer()

    def control(self):
        self.control_time += self.deltatime
        self.currentState = self.motorState.getData()
        default_pose_time = 2.0
        ratio = self.control_time / default_pose_time
        if ratio > 1: ratio = 1
        self.local_cmd_buffer = MotorCommand()

        for i in [0,G1_NUM_MOTOR-1]:
            self.local_cmd_buffer.q_target[i] = (1.0 - ratio) * self.currentState.q[i]

        self.motorCommand.setData(self.local_cmd_buffer)
    
    def writeLowCmd(self):
        command: MotorCommand = self.motorCommand.getData()
        lowCommand = lowcmd.LowCmd()

        for i in [0,G1_NUM_MOTOR-1]:
            lowCommand.motor_cmd[i].mode = 1;
            lowCommand.motor_cmd[i].tau = command.tau_ff[i];
            lowCommand.motor_cmd[i].q = command.q_target[i];
            lowCommand.motor_cmd[i].d = command.d_target[i];
            lowCommand.motor_cmd[i].dq = command.dq_target[i];
            lowCommand.motor_cmd[i].kp = command.kp[i];
            lowCommand.motor_cmd[i].kd = command.kd[i];

        

    def lowStateHandler(self, message : lowstate.LowState):
        msTmp = MotorState()

        for i in [0,G1_NUM_MOTOR-1]:
            msTmp.q[i] = message.motor_state[i].q;
            msTmp.dq[i] = message.motor_state[i].dq;

            if(message.motor_state[i].motorState != 0 and i <= G1JointIndex.RIGHT_ANKLE_ROLL):
                print("[ERROR] Motor ", i, " with code ", message.motor_state[i].motorState )

        self.motorState.setData(msTmp) 


def main(args=None):
    rclpy.init(args=args)

    wave_node = WaveNode()

    rclpy.spin(wave_node)

    wave_node.destroy_node()
    rclpy.shutdown()

__name__='__main__'
main()