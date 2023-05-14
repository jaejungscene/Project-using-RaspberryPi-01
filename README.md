"***Raspberry Pi 3 model B+***" is used.

<br>

# Practice
- **GPIO(General-Purpose Input/Output) pins**: GPIO pins are a physical interface between the Raspberry Pi and the outside world. By turning the current on and off on these pins, various sensors can be controlled. All the sensors I've controlled in this repository are [LED](https://eduino.kr/product/detail.html?product_no=69&cate_no=55&display_group=1), [alcohol sensor](https://eduino.kr/product/detail.html?product_no=275&cate_no=27&display_group=1), [pressure sensor](https://eduino.kr/product/detail.html?product_no=3012&gclid=CjwKCAjw6vyiBhB_EiwAQJRopm3G0-20C6xmRHMUuJ6J6qdD4bZ9KUkDILHxKMC_Rv6AqxwQtcdXOBoCApwQAvD_BwE), [weight sensor](https://eduino.kr/product/detail.html?product_no=325&cate_no=27&display_group=1), [speaker](https://eduino.kr/product/detail.html?product_no=453), [button](https://eduino.kr/product/detail.html?product_no=310&cate_no=51&display_group=1).
- **Soket Programming**

<br><br>

# Project - Public Electric Kickboard Safety System
## Purpose of Project
With the introduction of public electric kick scooters, there has been a rise in traffic accidents involving personal mobility devices, including electric kick scooters. Various factors, such as speeding and alcohol consumption, contribute to these accidents caused by public electric kick scooters. But the companies providing public electric kick scooters had only implemented a feature to detect and prevent speeding. To address this issue, the aim of the project is to develop additional features that detect not only speeding but also *instances of two-person riding* and *alcohol consumption*, thereby suggesting restrictions on such behaviors. Furthermore, this system is implemented to *automatically send accident signals to emergency services (112 or 119) in the event of a major accident*.



## Overview of Implemented System
<p align="center">
  - Overall System Abstraction -
  <br>
  <img width="901" alt="Screenshot 2023-05-14 at 3 36 57 PM" src="https://github.com/jaejungscene/Project-using-RaspberryPi-01/assets/88542073/be4e6730-7aeb-4cb3-b95d-500f8471aa6f">
</p>

There are three main functions to be implemented:
1. Conduct an alcohol test. If a rider fails the test, it makes the public electric kickboard be deactivated, while if they pass, the kickboard can be activated.
2. Detect if more than one person is riding. And if it's true, the systeme sound a warning tone through the speaker.
3. Detect whether the rider has been involved in a severe accident that exceeds the body's capacity, utilizing weight sensors and pressure sensors. If such an accident is detected, the system automatically reports it to the mock server (112 or 119 in the real world).

Based on above the three functions and the overall system abstraction, the complete logic is succinctly illustrated in the figure below.
<p align="center">
  <img width=80% height=80% alt="Screenshot 2023-05-14 at 4 39 24 PM" src="https://github.com/jaejungscene/Project-using-RaspberryPi-01/assets/88542073/6743fb90-f200-43ed-bfc7-2403c327f54e">

</p>
