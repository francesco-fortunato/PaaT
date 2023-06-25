# Concept 

![alt](img/iotworlds_dog_tracker-960x648.jpg)

## The Problem 

Owning a pet offers numerous health benefits, including increased exercise, outdoor activities, and social interaction. Pets provide companionship and can help alleviate feelings of loneliness and depression. However, losing a pet is an extremely distressing experience for their owners.

While the issue of lost pets is widely acknowledged, obtaining accurate statistics is challenging due to the lack of a national reporting system. In 2009, according to the American Humane Association **1 out of 3** pets become lost at some point in their lifetime (**33%**) and close to **10 million** dogs and cats are lost or stolen in the US every single year. According to the Coalition for Reuniting Pets and Families, **less than 23%** of lost pets in the United States are reunited with their owners.
We can synthesize this with:
- **over 135 millions of dogs and cats in the United State**
- **33% of ALL dogs and cats in the USA go MISSING**
- **more than 80% of missing pets are NEVER FOUND**

In 2016, [Peeva](https://peeva.co/blog/missing-pet-epidemic-facts-and-figures) conducted a survey sporadically over the course of several months in 2016 to get a more accurate gauge of the approximate percentage of owned dogs and cats in the United States that are reported lost, and of these reported lost pets (specifically dogs and cats), the percentage that are reclaimed by their owners (those that reported the pets as lost) by surveying a sample set indicative of the US pet owning population as a whole. The results and conclusions were surprising: the previous statistics reported are actually the same as their findings.
- **over 135 millions of dogs and cats in the United State**
- **33% of ALL dogs and cats in the USA go MISSING**
- **more than 80% of missing pets are NEVER FOUND**

Moreover, we conducted some studies on **HOW** pets were found. We found out that ID or Microchip, other "valid" elements, represents only the **2-10%** for helping find pets.

## A possible solution

A possible solution for the problem could be to use a GPS system. 
GPS system used for tracking pets already exists, but they presents some lacks:
- **The module cannot be disassembled**, so issues related to battery, radio or GPS malfunction cannot be fixed easily and the whole device must be replaced
- The recognized **location is not accurate** when there is **no affordable satellite signal** (and **no information** about this is given **to the end user**)
- There are **no actuators** that let the user find the pet in though scenarios, as in the night or in hidden places (bushes, walls, enclosed areas…)
- Sensors that allow user to know if his pet is steady for too long are missing (pet could have lost tracker or have health issues)
- An **history** of last day **positions** is missing
- GSM subscription is **costly** due to licencing (**€60/year**)

An **IoT GPS system**, insted, could provide:
- A **solution** for all the requrements above.
- **Improved Recovery Rates**: in case a pet gets lost, the tracking capability and low power consumption of significantly increases the chances of finding and recovering them.
- **Continuous Monitoring**: owners can access to information about their pet's movements and behavior patterns over time. 
- **Remote Access**: owners can remotely access their pet's location information through mobile applications or web interfaces.
- **Integration with Other Devices**: For example, it can be sync with smart home systems, allowing automated alerts to be sent to multiple devices or triggering actions like opening a pet door when the pet approaches. 
- **Data Analytics**: generate a wealth of data regarding a pet's activities, location history, and health metrics. This data can be analyzed to gain insights into the pet's behavior, exercise patterns, sleep quality, and more.

So, we based user requirements on the precedent ones (that were not satisfied) and on some interviews we made during the lessons.

## User Requirements

- It has to work for
  - **At least 24 hours**: in case we lose our pet it is important that the device remains active for at least one day, that could be a sufficient amount of time to track and retrieve it;
  - **At least one month**: in case we do not lose contact with our pet, it is sufficient that the device remains active for at least one month. This period of time can be used by the user if he will stay out for days, and it’s also a good trade-off between time required to retrieve the pet and efficiency consumption from a user point of view.

- Sampling time and promptly:
  - **5 min**: when we lose the contact with the pet, 5 minutes could be a sufficient amount of time to update the position, because in 5 minutes the pet could not do so much road, and the user can rescue it;

- Accuracy:
  - **10-15m**, that is a sufficient range in order to be able for the user to find his pet.
  - **False positive** and **false negative** has to be detectedable.

- Security: 
  - data exchanged by the application must be **encrypted** in order to hide sensitive information (like the position of the pet) to malicious attackers, providing **confidentiality** to the end user.

- Events: 
  - {"latitude": float, "longitude": float, “sat_num”: int}: since data transfer size are in the order of 2^7 bytes (+ headers) and since we need that the system has to be energy efficient that supports a communication in the order of ≈10 km, free, **LoRa** is strongly suitable for our system.

## Offered Services

The pet tracking system will have the following features:
- **Location Tracking**: The system utilizes GPS technology to track pets' location;
- **Geofencing**: Pet owners can set up a designated area for their pets, and the system will send an alert when the pet leaves the designated area;
- **Movement history**: The system keeps track of pets' movements and displays the data in a graphical format for pet owners to review;
- **Notifications**: Pet owners receive en email when their pets leave the designated area that incities the owner to access the site.

## Service Details
To use the pet tracking system, pet owners must first attach the GPS system to their pet's collar or harness. The system is powered by a battery, and the battery life will depend on the position of the pet, if it is in the geofence the device does not track the animal, on the other hand if the pet is outside the geofence, the device will update the position every 5 minutes.

Pet owners can access the system's features through a web interface. The web interface allows pet owners to view their pets' location, movement history, and set up geofencing. 

When pet owners set up geofencing, they can define a specific area where their pet is allowed to roam. If the pet leaves the designated area, the system sends an alert to the pet owner's email address. Pet owners can then use the web interface to track their pet's location and retrieve their pet.

## Conclusion
Pet tracking is a crucial aspect of pet ownership, and this IoT Pet Tracking System can provide a reliable and efficient solution for pet owners. With location tracking, geofencing, movement history, and notifications, pet owners can keep an eye on their pets' location and ensure their pets' safety, improve recovery rates, and so o.
