import { Component } from '@angular/core';
import * as L from 'leaflet';
import { PetService } from '../pet.service';

@Component({
  selector: 'app-map',
  templateUrl: './map.component.html',
  styleUrls: ['./map.component.css'],
})
export class MapComponent {
  private map: L.Map | undefined;
  private gps: L.Marker | undefined;
  private pet: L.Marker | undefined;
  private geoFence: L.Polygon<any> | undefined;
  private gpsIcon = L.icon({
    iconUrl: 'assets/gps.png',
    iconSize: [40, 40],
    iconAnchor: [20, 20],
  });
  private petIcon = L.icon({
    iconUrl: 'assets/pet.png',
    iconSize: [40, 40],
    iconAnchor: [20, 20],
  });

  private initMap(): void {
    this.map = L.map('map', {
      center: [41.891106645704795, 12.503608718298109],
      zoom: 18,
      attributionControl: false,
    });

    const tiles = L.tileLayer(
      'https://{s}.tile.openstreetmap.org/{z}/{x}/{y}.png',
      {
        maxZoom: 19,
        minZoom: 5,
      }
    );

    tiles.addTo(this.map);

    this.geoFence = L.polygon(this.petService.getGeofence(), {
      color: 'green',
      fillColor: 'green',
      fillOpacity: 0.3,
    });
    this.geoFence.addTo(this.map);
  }

  constructor(private petService: PetService) {}

  ngAfterViewInit(): void {
    this.initMap();
    if (this.map) {
      this.initGps();
      this.initPet();
      this.trackPet();
      this.trackGps();
    }
  }

  initGps(): void {
    if (this.map) {
      this.map.locate({ watch: true, enableHighAccuracy: true });
      this.map.on('locationfound', (e) => {
        if (!this.gps && this.map) {
          this.gps = L.marker(e.latlng, {
            icon: this.gpsIcon,
          });
          this.gps.addTo(this.map);
          this.map.flyTo(e.latlng, 18);
        }
      });
    }
  }

  initPet(): void {
    if (this.map) {
      this.pet = L.marker(this.petService.getPetPosition(), {
        icon: this.petIcon,
      });
      this.pet.bindPopup(
        'Last position update: ' +
          new Date(this.petService.getPetPosition().timestamp).toLocaleString(
            'it'
          )
      );
      this.pet.addTo(this.map);
    }
  }

  gotoGps(): void {
    if (this.map && this.gps) {
      this.map.flyTo(this.gps.getLatLng(), 18);
    }
  }

  gotoPet(): void {
    if (this.map && this.pet) {
      this.map.flyTo(this.pet.getLatLng(), 18);
    }
  }

  // update periodically the pet position
  trackPet(): void {
    setInterval(() => {
      if (this.pet) {
        this.pet.setLatLng(this.petService.getPetPosition());
        this.pet.bindPopup(
          'Last position update: ' +
            new Date(this.petService.getPetPosition().timestamp).toLocaleString(
              'it'
            )
        );
      }
    }, 10000);
  }
  //update periodically the gps position
  trackGps(): void {
    setInterval(() => {
      if (this.map) {
        this.map.locate({ watch: true, enableHighAccuracy: true });
        this.map.on('locationfound', (e) => {
          if (this.gps) {
            this.gps.setLatLng(e.latlng);
          }
        });
      }
    }, 10000);
  }
}
