import { Component } from '@angular/core';
import * as L from 'leaflet';
import { PetService } from '../pet.service';
import '@geoman-io/leaflet-geoman-free';

@Component({
  selector: 'app-map',
  templateUrl: './map.component.html',
  styleUrls: ['./map.component.css'],
})
export class MapComponent {
  private map: L.Map | undefined;
  gps: L.Marker | undefined;
  pet: L.Marker | undefined;
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
  editingFence: boolean = false;
  fenceMenuOpen: boolean = false;
  lightOn: boolean = false;
  soundOn: boolean = false;
  timelineOpen: boolean = false;
  petStatus:
    | {
        online: boolean;
        position: { lat: number; lng: number };
        timestamp: number;
      }
    | undefined;

  private initMap(): void {
    this.map = L.map('map', {
      center: [41.891106645704795, 12.503608718298109],
      zoom: 18,
      attributionControl: false,
      zoomControl: false,
    });

    L.control
      .zoom({
        position: 'topright',
      })
      .addTo(this.map);

    const tiles = L.tileLayer(
      'https://{s}.tile.openstreetmap.org/{z}/{x}/{y}.png',
      {
        maxZoom: 19,
        minZoom: 5,
      }
    );

    tiles.addTo(this.map);

    this.geoFence = L.polygon(this.petService.getGeofence(), {
      fillOpacity: 0.3,
    });
    this.geoFence.addTo(this.map);
  }

  constructor(private petService: PetService) {}

  ngAfterViewInit(): void {
    this.initMap();
    if (this.map) {
      this.initPet();

      this.initGps();
      this.trackPet();
      this.trackGps();
      // this.lightOn =  this.petService.lightStatus();
      // this.soundOn = this.petService.soundStatus();
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
        }
      });
    }
  }

  initPet(): void {
    this.petStatus = this.petService.getPetStatus();
    if (this.map && this.petStatus?.online) {
      this.pet = L.marker(this.petStatus.position, {
        icon: this.petIcon,
      });
      this.pet.bindPopup(
        'Last position update: ' +
          new Date(this.petStatus.timestamp).toLocaleString('it')
      );
      this.pet.addTo(this.map);

      this.gotoPet();
    }
  }

  gotoGps(): void {
    if (this.map && this.gps) {
      this.map.flyTo(this.gps.getLatLng(), 18);
    } else {
      alert(
        'PaaT cannot access your location. Please turn on GPS and check your browser settings.'
      );
    }
  }

  gotoPet(): void {
    if (this.map && this.pet) {
      this.map.flyTo(this.pet.getLatLng(), 18);
    } else {
      alert('PaaT cannot access actual Pet location.');
    }
  }

  toggleLight(): void {
    if (this.lightOn) {
      this.lightOn = false;
      // this.petService.lightOff();
    } else {
      this.lightOn = true;
      // this.petService.lightOn();
    }
  }

  toggleSound(): void {
    if (this.soundOn) {
      this.soundOn = false;
      //   this.petService.soundOff();
    } else {
      this.soundOn = true;
      //   this.petService.soundOn();
    }
  }

  toggleTimeline(): void {
    this.timelineOpen = !this.timelineOpen;
  }

  toggleFenceOptions(): void {
    if (this.map) {
      if (!this.geoFence) {
        if (this.editingFence) {
          this.editingFence = false;
          this.map.pm.disableGlobalEditMode();
        } else {
          this.fenceMenuOpen = false;
          this.editingFence = true;
          this.map.pm.enableDraw('Polygon', {});
          this.map.on('pm:create', (e) => {
            if (this.map) {
              if (e.layer instanceof L.Polygon) {
                this.geoFence = e.layer;
                this.geoFence.addTo(this.map);
                this.editingFence = false;
              }
            }
          });
        }
      } else {
        if (this.editingFence) {
          this.map.pm.disableGlobalEditMode();
          this.editingFence = false;
          const latLng = this.geoFence.getLatLngs();
          console.log(latLng);
          // this.petService.saveGeofence(this.geoFence.getLatLngs());
        } else if (this.fenceMenuOpen) {
          this.fenceMenuOpen = false;
        } else {
          this.fenceMenuOpen = true;
        }
      }
    }
  }

  editFence(): void {
    this.fenceMenuOpen = false;
    this.editingFence = true;
    if (this.map && this.geoFence) {
      this.geoFence.pm.enable();
    }
  }

  removeFence(): void {
    this.fenceMenuOpen = false;
    if (this.map && this.geoFence) {
      this.map.removeLayer(this.geoFence);
      this.geoFence = undefined;
      this.petService.removeGeofence();
    }
  }

  // update periodically the pet position
  trackPet(): void {
    setInterval(() => {
      this.petStatus = this.petService.getPetStatus();
      if (this.petStatus?.online) {
        if (!this.pet) {
          this.pet = L.marker(this.petStatus.position, {
            icon: this.petIcon,
          });
        } else {
          this.pet.setLatLng(this.petStatus.position);
        }
        this.pet.bindPopup(
          'Last position update: ' +
            new Date(this.petStatus.timestamp).toLocaleString('it')
        );
      } else {
        this.pet = undefined;
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
        this.map.once('locationerror', (e) => {
          this.gps = undefined;
        });
      }
    }, 10000);
  }
}
