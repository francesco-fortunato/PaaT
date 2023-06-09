import { HttpClient } from '@angular/common/http';
import { Injectable } from '@angular/core';
import { Observable } from 'rxjs';
import * as L from 'leaflet';

@Injectable({
  providedIn: 'root',
})
export class PetService {
  constructor(private http: HttpClient) {}

  getPetStatus(id: number): Observable<{
    Light: string;
    Sound: string;
    sample_time: number;
    Longitude: string;
    Latitude: string;
    device_id: number;
  }> {
    return this.http.get<{
      Light: string;
      Sound: string;
      sample_time: number;
      Longitude: string;
      Latitude: string;
      device_id: number;
    }>(
      'https://wuufd3nn7k.execute-api.us-east-1.amazonaws.com/beta/get-actual-data?id=' +
        id
    );
  }

  getGeofence(id: number): Observable<{
    device_id: number;
    device_data: L.LatLng[][];
    sample_time: number;
  }> {
    return this.http.get<{
      device_id: number;
      device_data: L.LatLng[][];
      sample_time: number;
    }>(
      'https://wuufd3nn7k.execute-api.us-east-1.amazonaws.com/beta/geofence?id=' +
        id
    );
  }

  saveGeofence(
    id: number,
    latLngList: L.LatLng[] | L.LatLng[][] | L.LatLng[][][]
  ): Observable<any> {
    return this.http.post(
      'https://wuufd3nn7k.execute-api.us-east-1.amazonaws.com/beta/geofence?id=' +
        id,
      latLngList
    );
  }

  // removeGeofence(id: number): Observable<any> {
  //   return this.http.delete(
  //     'https://wuufd3nn7k.execute-api.us-east-1.amazonaws.com/beta/geofence?id=' +
  //       id
  //   );
  // }

  getLatestPetPath(id: number): Observable<any> {
    return this.http.get(
      'https://wuufd3nn7k.execute-api.us-east-1.amazonaws.com/beta/location-history?id=' +
        id
    );
  }

  setLightOrSound(id: number, light: boolean, sound: boolean): Observable<any> {
    return this.http.get(
      'https://wuufd3nn7k.execute-api.us-east-1.amazonaws.com/beta/set-light-and-sound?light=' +
        (light ? 'y' : 'n') +
        '&buzz=' +
        (sound ? 'y' : 'n') +
        '&id=' +
        id
    );
  }
}
