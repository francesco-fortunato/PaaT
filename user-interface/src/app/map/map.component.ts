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
    });

    const tiles = L.tileLayer(
      'https://{s}.tile.openstreetmap.org/{z}/{x}/{y}.png',
      {
        maxZoom: 19,
        minZoom: 5,
      }
    );

    tiles.addTo(this.map);
  }

  constructor(private petService: PetService) {}

  ngAfterViewInit(): void {
    this.initMap();
    if (this.map) {
      this.gotoGps();
    }
  }

  gotoGps(): void {
    if (this.map) {
      if (!this.gps) {
        this.map.locate({ watch: true, enableHighAccuracy: true });
        this.map.on('locationfound', (e) => {
          if (!this.gps && this.map) {
            this.gps = L.marker(e.latlng, {
              icon: this.gpsIcon,
            });
            this.gps.addTo(this.map);
            this.map.flyTo(e.latlng, 18);
          } else if (this.gps) {
            this.gps.setLatLng(e.latlng);
          }
        });
      } else {
        this.map.flyTo(this.gps.getLatLng(), 18);
      }
    }
  }

  gotoPet(): void {
    if (this.map) {
      if (!this.pet) {
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
        this.map.flyTo(this.pet.getLatLng(), 18);
      } else {
        this.map.flyTo(this.pet.getLatLng(), 18);
      }
    }
  }
}
