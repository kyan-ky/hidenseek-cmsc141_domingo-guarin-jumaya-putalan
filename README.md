# **Automa-tag: Ryan's Revenge**
A project for CMSC 141 
Members: 
- Domingo, Keanne
- Guarin, Nicolete
- Jumaya, Shane
- Putalan, Tammy

---

### **Introduction/Rationale** 
This project aims to recreate Saksak Baril as a game where the player takes the role of the Seeker, and the Hiders are controlled by Finite State Machine AI. Using an FSM allows for realistic Hider behavior, such as running, evading, and hiding from the Player/Seeker. This game involves stealth, awareness, and suspense; with the usage of FSMs it simulates and mimics the behavior of how Hiders would behave in real life making for an engaging and fun game to play, while recreating the traditional Filipino childhood game.

---

### **Detailed Description**
Saksak Baril is a Filipino children’s game that is basically Hide-and-Seek but with a twist. There is a Seeker who must find and tag all the Hiders, the Hiders must hide until the time limit is exhausted. The added twist is that the Seeker holds the “baril” of which they must use to tag the Hiders, while the Hiders each could “saksak” the Seeker. The Seeker must be able to find all the Hiders while at the same time, avoiding being “saksak”-ed by the Hiders.

### **How to Play or Use**
*Player Role: Seeker*
- The player will be the seeker finding all the Hider Bots.
    - The player will lose if:
        - Hiders touch the player from behind.
        - Hiders who are out of sight touch the player.

*AI Bots Role: Hider*
- Bots who are to hide from the seeker with the goal of winning from elapsed time or successfully touching the seeker when out of sight.

*Game Phase*
1. Hiding Phase
    - Countdown for hiders to scatter and find spots to hide.
    - Seeker cannot move until the countdown ends.
2. Seeking Phase
    - You must explore and successfully tag all the Hiders.
    - You must watch your back and avoid losing through being tagged from behind.
3. End Game Condition
    - Win – all the hiders are tagged.
    - Lose – Timer runs out or a hider touches you from behind.



