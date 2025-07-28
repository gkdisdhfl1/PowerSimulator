# PowerSimulator 프로젝트 과제 내용

## 과제 1. 입력값을 그래프로 표시하기
- 입력값 지정
- 시간간격 지정
- 초단위 입력. 최소값 : 0.1초
- 지정된 시간마다 입력값을 캡쳐하여 vector에 추가 (적절한 container 선택)
- 시간값과 입력값의 pair로 보관
- 최대 개수 제한 (1000개, 10000개 등으로 테스트)
- 최대 개수 초과 시, 오래된 데이터부터 삭제
- 캡쳐 된 입력값을 graph로 표현

## 과제 2. UI 편의성 향상
- 입력값을 편하게 조정할 수 있는 slider widget 추가
- Slider의 최소값은 -500, 최대값은 500
- 직접 입력하는 widget과 slider를 통한 입력 모두 가능
- 한 쪽에서 입력을 변경하면 다른 쪽에 반영 (slider <-> lineedit)
- Slider와 LineEdit을 하나의 widget으로 합한 widget 구현
- Graph의 최대 폭을 초단위로 설정

## 과제 3. AC(교류) 파형 생성
- 다이얼 입력기 만들기
- 입력 위치를 각도로 환산하여 출력 (0 ~ 360도. 0도 = 360도)
- 이전의 입력값에 다이얼의 각도에 해당하는 sin 값을 곱하여 사용
- 수동으로 다이얼 회전 시, 교류 파형 생성 확인
- 자동 회전기능 추가
- 회전 속도 조절 (초 단위 회전 주기)
- 값을 읽는 시간 간격마다 각도 갱신

## 과제 4. 인터페이스 조정
- Source 파형에 대한 설정
- 초당 cycle 수 (주파수)
- Sampling의 초당 cycle 수
- Sampling의 cycle 당 sample 수

### Simulation 예
- Source 설정 : 초당 20 cycle
- Sampling 설정 : 초당 10 cycle, Cycle 당 10 sample
- 10ms 마다 1회 sampling
- Sampling 1 cycle 당 source 파형 2개씩 포함됨
- Sampling 마다 Source 파형에서, 10ms가 흘러간 것으로 계산
- 이전 시간에서 10ms를 더하는 방식
- QTimer의 시간오차를 회피하기 위한 방법
- 10ms마다 source 파형에서는 angle 72도씩 지나감 (360 : 50 = x : 10 -> x = 72)

## 과제 5. 시간 조절하기
- 시간 비율 조절기능
- 50Hz (1초에 50cycle) sampling 시뮬레이션
- Cycle 당 20ms
- Cycle당 16 sampling 한다고 가정할 때, Sampling당 1.25ms 간격
- 1초에 1 cycle 시뮬레이션 하고싶다면 50배 배율로 계산하면 됨 (20ms : 1000ms = 1 : 50)
- 1.25ms x 50 = 62.5ms마다 sampling 하면 됨
- Simulation 상의 시간은 1.25ms가 흘러간 것으로 계산
- QTimer : 1ms 단위로 설정 가능하므로 63ms마다 1sample 하도록 scheduling 함
- 시간 비율 입력 (숫자 또는 slider로 입력)
